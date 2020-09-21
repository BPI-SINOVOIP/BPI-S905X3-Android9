/*
 * Copyright (c) 2001, 2005, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
/*
 * This module tracks classes that have been prepared, so as to
 * be able to compute which have been unloaded.  On VM start-up
 * all prepared classes are put in a table.  As class prepare
 * events come in they are added to the table.  After an unload
 * event or series of them, the VM can be asked for the list
 * of classes; this list is compared against the table keep by
 * this module, any classes no longer present are known to
 * have been unloaded.
 *
 * ANDROID-CHANGED: This module is almost totally re-written
 * for android. On android, we have a limited number of jweak
 * references that can be around at any one time. In order to
 * preserve this limited resource for user-code use we keep
 * track of the status of classes using JVMTI tags.
 *
 * We keep a linked-list of the signatures of loaded classes
 * associated with the tag we gave to that class. The tag is
 * simply incremented every time we add a new class.
 *
 * We also request (on the separate tracking jvmtiEnv) an
 * ObjectFree event be called for each of these classes. This
 * allows us to keep a running list of all the classes known to
 * have been collected since the last call to
 * classTrack_processUnloads. On each call to processUnloads we
 * iterate through this list and remove from the main list all
 * the objects that have been collected. We then return a list of
 * the class-signatures that have been collected.
 *
 * For efficiency and simplicity we don't bother retagging or
 * re-using old tags, instead relying on the fact that no
 * program will ever be able to exhaust the (2^64 - 1) possible
 * tag values (which would require that many class-loads).
 *
 * This relies on the tagging and ObjectFree implementation being
 * relatively efficient for performance. It has the advantage of
 * not requiring any jweaks.
 *
 * All calls into any function of this module must be either
 * done before the event-handler system is setup or done while
 * holding the event handlerLock. The list of freed classes is
 * protected by the classTagLock.
 */

#include "util.h"
#include "bag.h"
#include "classTrack.h"

typedef struct KlassNode {
    jlong klass_tag;         /* Tag the klass has in the tracking-env */
    char *signature;         /* class signature */
    struct KlassNode *next;  /* next node in this slot */
} KlassNode;

/*
 * pointer to first node of a linked list of prepared classes KlassNodes.
 */
static KlassNode *list;

/*
 * The JVMTI env we use to keep track of klass tags which allows us to detect class-unloads.
 */
static jvmtiEnv *trackingEnv;

/*
 * The current highest tag number in use by the trackingEnv.
 *
 * No need for synchronization since everything is done under the handlerLock.
 */
static jlong currentKlassTag;

/*
 * A lock to protect access to 'deletedTagBag'
 */
static jrawMonitorID deletedTagLock;

/*
 * A bag containing all the deleted klass_tags ids. This must be accessed under the
 * deletedTagLock.
 *
 * It is cleared each time classTrack_processUnloads is called.
 */
struct bag* deletedTagBag;

/*
 * The callback for when classes are freed. Only classes are called because this is registered with
 * the trackingEnv which only tags classes.
 */
static void JNICALL
cbTrackingObjectFree(jvmtiEnv* jvmti_env, jlong tag)
{
    debugMonitorEnter(deletedTagLock);
    *(jlong*)bagAdd(deletedTagBag) = tag;
    debugMonitorExit(deletedTagLock);
}

/*
 * Returns true (thus continuing the iteration) if the item is not the searched for tag.
 */
static jboolean
isNotTag(void* item, void* needle)
{
    return *(jlong*)item != *(jlong*)needle;
}

/*
 * This requires that deletedTagLock and the handlerLock are both held.
 */
static jboolean
isClassUnloaded(jlong tag)
{
    /* bagEnumerateOver returns true if 'func' returns true on all items and aborts early if not. */
    return !bagEnumerateOver(deletedTagBag, isNotTag, &tag);
}

/*
 * Called after class unloads have occurred.  Creates a new hash table
 * of currently loaded prepared classes.
 * The signatures of classes which were unloaded (not present in the
 * new table) are returned.
 *
 * NB This relies on addPreparedClass being called for every class loaded after the
 * classTrack_initialize function is called. We will not request all loaded classes again after
 * that. It also relies on not being called concurrently with any classTrack_addPreparedClass or
 * other classTrack_processUnloads calls.
 */
struct bag *
classTrack_processUnloads(JNIEnv *env)
{
    /* We could optimize this somewhat by holding the deletedTagLock for a much shorter time,
     * replacing it as soon as we enter and then destroying it once we are done with it. This will
     * cause a lot of memory churn and this function is not expected to be called that often.
     * Furthermore due to the check for an empty bag (which should be very common) normally this
     * will finish very quickly. In cases where there is a concurrent GC occuring and a class is
     * being collected the GC-ing threads could be blocked until we are done but this is expected to
     * be very rare.
     */
    debugMonitorEnter(deletedTagLock);
    /* Take and return the deletedTagBag */
    struct bag* deleted = bagCreateBag(sizeof(char*), bagSize(deletedTagBag));
    /* The deletedTagBag is going to be much shorter than the klassNode list so we should walk the
     * KlassNode list once and scan the deletedTagBag each time. We only need to this in the rare
     * case that there was anything deleted though.
     */
    if (bagSize(deletedTagBag) != 0) {
        KlassNode* node = list;
        KlassNode** previousNext = &list;

        while (node != NULL) {
            if (isClassUnloaded(node->klass_tag)) {
                /* Update the previous node's next pointer to point after this node. Note that we
                 * update the value pointed to by previousNext but not the value of previousNext
                 * itself.
                 */
                *previousNext = node->next;
                /* Put this nodes signature into the deleted bag */
                *(char**)bagAdd(deleted) = node->signature;
                /* Deallocate the node */
                jvmtiDeallocate(node);
            } else {
                /* This node will become the previous node so update the previousNext pointer to
                 * this nodes next pointer.
                 */
                previousNext = &(node->next);
            }
            node = *previousNext;
        }
        bagDeleteAll(deletedTagBag);
    }
    debugMonitorExit(deletedTagLock);
    return deleted;
}

/*
 * Add a class to the prepared class list.
 * Assumes no duplicates.
 */
void
classTrack_addPreparedClass(JNIEnv *env, jclass klass)
{
    KlassNode *node;
    jvmtiError error;

    if (gdata->assertOn) {
        /* Check this is not a duplicate */
        jlong tag;
        error = JVMTI_FUNC_PTR(trackingEnv,GetTag)(trackingEnv, klass, &tag);
        if (error != JVMTI_ERROR_NONE) {
            EXIT_ERROR(error,"unable to get-tag with class trackingEnv!");
        }
        if (tag != 0l) {
            JDI_ASSERT_FAILED("Attempting to insert duplicate class");
        }
    }

    node = jvmtiAllocate(sizeof(KlassNode));
    if (node == NULL) {
        EXIT_ERROR(AGENT_ERROR_OUT_OF_MEMORY,"KlassNode");
    }
    error = classSignature(klass, &(node->signature), NULL);
    if (error != JVMTI_ERROR_NONE) {
        jvmtiDeallocate(node);
        EXIT_ERROR(error,"signature");
    }
    node->klass_tag = ++currentKlassTag;
    error = JVMTI_FUNC_PTR(trackingEnv,SetTag)(trackingEnv, klass, node->klass_tag);
    if (error != JVMTI_ERROR_NONE) {
        jvmtiDeallocate(node->signature);
        jvmtiDeallocate(node);
        EXIT_ERROR(error,"SetTag");
    }

    /* Insert the new node */
    node->next = list;
    list = node;
}

static jboolean
setupEvents()
{
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_generate_object_free_events = 1;
    jvmtiError error = JVMTI_FUNC_PTR(trackingEnv,AddCapabilities)(trackingEnv, &caps);
    if (error != JVMTI_ERROR_NONE) {
        return JNI_FALSE;
    }
    jvmtiEventCallbacks cb;
    memset(&cb, 0, sizeof(cb));
    cb.ObjectFree = cbTrackingObjectFree;
    error = JVMTI_FUNC_PTR(trackingEnv,SetEventCallbacks)(trackingEnv, &cb, sizeof(cb));
    if (error != JVMTI_ERROR_NONE) {
        return JNI_FALSE;
    }
    error = JVMTI_FUNC_PTR(trackingEnv,SetEventNotificationMode)
            (trackingEnv, JVMTI_ENABLE, JVMTI_EVENT_OBJECT_FREE, NULL);
    if (error != JVMTI_ERROR_NONE) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

/*
 * Called once to build the initial prepared class hash table.
 */
void
classTrack_initialize(JNIEnv *env)
{
    /* ANDROID_CHANGED: Setup the tracking env and the currentKlassTag */
    trackingEnv = getSpecialJvmti();
    if ( trackingEnv == NULL ) {
        EXIT_ERROR(AGENT_ERROR_INTERNAL,"Failed to allocate tag-tracking jvmtiEnv");
    }
    /* We want to create these before turning on the events or tagging anything. */
    deletedTagLock = debugMonitorCreate("Deleted class tag lock");
    deletedTagBag = bagCreateBag(sizeof(jlong), 10);
    /* ANDROID-CHANGED: Setup the trackingEnv's ObjectFree event */
    if (!setupEvents()) {
        /* On android classes are usually not unloaded too often so this is not a huge loss. */
        ERROR_MESSAGE(("Unable to setup class ObjectFree tracking! Class unloads will not "
                       "be reported!"));
    }
    currentKlassTag = 0l;
    list = NULL;
    WITH_LOCAL_REFS(env, 1) {

        jint classCount;
        jclass *classes;
        jvmtiError error;
        jint i;

        error = allLoadedClasses(&classes, &classCount);
        if ( error == JVMTI_ERROR_NONE ) {
            for (i=0; i<classCount; i++) {
                jclass klass = classes[i];
                jint status;
                jint wanted =
                    (JVMTI_CLASS_STATUS_PREPARED|JVMTI_CLASS_STATUS_ARRAY);

                /* We only want prepared classes and arrays */
                status = classStatus(klass);
                if ( (status & wanted) != 0 ) {
                    classTrack_addPreparedClass(env, klass);
                }
            }
            jvmtiDeallocate(classes);
        } else {
            EXIT_ERROR(error,"loaded classes array");
        }

    } END_WITH_LOCAL_REFS(env)

}

void
classTrack_reset(void)
{
}
