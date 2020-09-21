/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2006
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file arrayobj.c
 *
 * Implementation of Vertex Array Objects (VAOs), from OpenGL 3.1+,
 * the GL_ARB_vertex_array_object extension, or the older
 * GL_APPLE_vertex_array_object extension.
 *
 * \todo
 * The code in this file borrows a lot from bufferobj.c.  There's a certain
 * amount of cruft left over from that origin that may be unnecessary.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 * \author Brian Paul
 */


#include "glheader.h"
#include "hash.h"
#include "image.h"
#include "imports.h"
#include "context.h"
#include "bufferobj.h"
#include "arrayobj.h"
#include "macros.h"
#include "mtypes.h"
#include "varray.h"
#include "main/dispatch.h"
#include "util/bitscan.h"


/**
 * Look up the array object for the given ID.
 *
 * \returns
 * Either a pointer to the array object with the specified ID or \c NULL for
 * a non-existent ID.  The spec defines ID 0 as being technically
 * non-existent.
 */

struct gl_vertex_array_object *
_mesa_lookup_vao(struct gl_context *ctx, GLuint id)
{
   if (id == 0)
      return NULL;
   else
      return (struct gl_vertex_array_object *)
         _mesa_HashLookup(ctx->Array.Objects, id);
}


/**
 * Looks up the array object for the given ID.
 *
 * Unlike _mesa_lookup_vao, this function generates a GL_INVALID_OPERATION
 * error if the array object does not exist. It also returns the default
 * array object when ctx is a compatibility profile context and id is zero.
 */
struct gl_vertex_array_object *
_mesa_lookup_vao_err(struct gl_context *ctx, GLuint id, const char *caller)
{
   /* The ARB_direct_state_access specification says:
    *
    *    "<vaobj> is [compatibility profile:
    *     zero, indicating the default vertex array object, or]
    *     the name of the vertex array object."
    */
   if (id == 0) {
      if (ctx->API == API_OPENGL_CORE) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "%s(zero is not valid vaobj name in a core profile "
                     "context)", caller);
         return NULL;
      }

      return ctx->Array.DefaultVAO;
   } else {
      struct gl_vertex_array_object *vao;

      if (ctx->Array.LastLookedUpVAO &&
          ctx->Array.LastLookedUpVAO->Name == id) {
         vao = ctx->Array.LastLookedUpVAO;
      } else {
         vao = (struct gl_vertex_array_object *)
            _mesa_HashLookup(ctx->Array.Objects, id);

         /* The ARB_direct_state_access specification says:
          *
          *    "An INVALID_OPERATION error is generated if <vaobj> is not
          *     [compatibility profile: zero or] the name of an existing
          *     vertex array object."
          */
         if (!vao || !vao->EverBound) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "%s(non-existent vaobj=%u)", caller, id);
            return NULL;
         }

         _mesa_reference_vao(ctx, &ctx->Array.LastLookedUpVAO, vao);
      }

      return vao;
   }
}


/**
 * For all the vertex binding points in the array object, unbind any pointers
 * to any buffer objects (VBOs).
 * This is done just prior to array object destruction.
 */
static void
unbind_array_object_vbos(struct gl_context *ctx, struct gl_vertex_array_object *obj)
{
   GLuint i;

   for (i = 0; i < ARRAY_SIZE(obj->BufferBinding); i++)
      _mesa_reference_buffer_object(ctx, &obj->BufferBinding[i].BufferObj, NULL);

   for (i = 0; i < ARRAY_SIZE(obj->_VertexAttrib); i++)
      _mesa_reference_buffer_object(ctx, &obj->_VertexAttrib[i].BufferObj, NULL);
}


/**
 * Allocate and initialize a new vertex array object.
 */
struct gl_vertex_array_object *
_mesa_new_vao(struct gl_context *ctx, GLuint name)
{
   struct gl_vertex_array_object *obj = CALLOC_STRUCT(gl_vertex_array_object);
   if (obj)
      _mesa_initialize_vao(ctx, obj, name);
   return obj;
}


/**
 * Delete an array object.
 */
void
_mesa_delete_vao(struct gl_context *ctx, struct gl_vertex_array_object *obj)
{
   unbind_array_object_vbos(ctx, obj);
   _mesa_reference_buffer_object(ctx, &obj->IndexBufferObj, NULL);
   mtx_destroy(&obj->Mutex);
   free(obj->Label);
   free(obj);
}


/**
 * Set ptr to vao w/ reference counting.
 * Note: this should only be called from the _mesa_reference_vao()
 * inline function.
 */
void
_mesa_reference_vao_(struct gl_context *ctx,
                     struct gl_vertex_array_object **ptr,
                     struct gl_vertex_array_object *vao)
{
   assert(*ptr != vao);

   if (*ptr) {
      /* Unreference the old array object */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_vertex_array_object *oldObj = *ptr;

      mtx_lock(&oldObj->Mutex);
      assert(oldObj->RefCount > 0);
      oldObj->RefCount--;
      deleteFlag = (oldObj->RefCount == 0);
      mtx_unlock(&oldObj->Mutex);

      if (deleteFlag)
         _mesa_delete_vao(ctx, oldObj);

      *ptr = NULL;
   }
   assert(!*ptr);

   if (vao) {
      /* reference new array object */
      mtx_lock(&vao->Mutex);
      if (vao->RefCount == 0) {
         /* this array's being deleted (look just above) */
         /* Not sure this can every really happen.  Warn if it does. */
         _mesa_problem(NULL, "referencing deleted array object");
         *ptr = NULL;
      }
      else {
         vao->RefCount++;
         *ptr = vao;
      }
      mtx_unlock(&vao->Mutex);
   }
}


/**
 * Initialize attribtes of a vertex array within a vertex array object.
 * \param vao  the container vertex array object
 * \param index  which array in the VAO to initialize
 * \param size  number of components (1, 2, 3 or 4) per attribute
 * \param type  datatype of the attribute (GL_FLOAT, GL_INT, etc).
 */
static void
init_array(struct gl_context *ctx,
           struct gl_vertex_array_object *vao,
           GLuint index, GLint size, GLint type)
{
   struct gl_array_attributes *array = &vao->VertexAttrib[index];
   struct gl_vertex_buffer_binding *binding = &vao->BufferBinding[index];

   array->Size = size;
   array->Type = type;
   array->Format = GL_RGBA; /* only significant for GL_EXT_vertex_array_bgra */
   array->Stride = 0;
   array->Ptr = NULL;
   array->RelativeOffset = 0;
   array->Enabled = GL_FALSE;
   array->Normalized = GL_FALSE;
   array->Integer = GL_FALSE;
   array->Doubles = GL_FALSE;
   array->_ElementSize = size * _mesa_sizeof_type(type);
   array->BufferBindingIndex = index;

   binding->Offset = 0;
   binding->Stride = array->_ElementSize;
   binding->BufferObj = NULL;
   binding->_BoundArrays = BITFIELD64_BIT(index);

   /* Vertex array buffers */
   _mesa_reference_buffer_object(ctx, &binding->BufferObj,
                                 ctx->Shared->NullBufferObj);
}


/**
 * Initialize a gl_vertex_array_object's arrays.
 */
void
_mesa_initialize_vao(struct gl_context *ctx,
                     struct gl_vertex_array_object *vao,
                     GLuint name)
{
   GLuint i;

   vao->Name = name;

   mtx_init(&vao->Mutex, mtx_plain);
   vao->RefCount = 1;

   /* Init the individual arrays */
   for (i = 0; i < ARRAY_SIZE(vao->VertexAttrib); i++) {
      switch (i) {
      case VERT_ATTRIB_WEIGHT:
         init_array(ctx, vao, VERT_ATTRIB_WEIGHT, 1, GL_FLOAT);
         break;
      case VERT_ATTRIB_NORMAL:
         init_array(ctx, vao, VERT_ATTRIB_NORMAL, 3, GL_FLOAT);
         break;
      case VERT_ATTRIB_COLOR1:
         init_array(ctx, vao, VERT_ATTRIB_COLOR1, 3, GL_FLOAT);
         break;
      case VERT_ATTRIB_FOG:
         init_array(ctx, vao, VERT_ATTRIB_FOG, 1, GL_FLOAT);
         break;
      case VERT_ATTRIB_COLOR_INDEX:
         init_array(ctx, vao, VERT_ATTRIB_COLOR_INDEX, 1, GL_FLOAT);
         break;
      case VERT_ATTRIB_EDGEFLAG:
         init_array(ctx, vao, VERT_ATTRIB_EDGEFLAG, 1, GL_BOOL);
         break;
      case VERT_ATTRIB_POINT_SIZE:
         init_array(ctx, vao, VERT_ATTRIB_POINT_SIZE, 1, GL_FLOAT);
         break;
      default:
         init_array(ctx, vao, i, 4, GL_FLOAT);
         break;
      }
   }

   _mesa_reference_buffer_object(ctx, &vao->IndexBufferObj,
                                 ctx->Shared->NullBufferObj);
}


/**
 * Add the given array object to the array object pool.
 */
static void
save_array_object(struct gl_context *ctx, struct gl_vertex_array_object *vao)
{
   if (vao->Name > 0) {
      /* insert into hash table */
      _mesa_HashInsert(ctx->Array.Objects, vao->Name, vao);
   }
}


/**
 * Remove the given array object from the array object pool.
 * Do not deallocate the array object though.
 */
static void
remove_array_object(struct gl_context *ctx, struct gl_vertex_array_object *vao)
{
   if (vao->Name > 0) {
      /* remove from hash table */
      _mesa_HashRemove(ctx->Array.Objects, vao->Name);
   }
}


/**
 * Updates the derived gl_vertex_arrays when a gl_vertex_attrib_array
 * or a gl_vertex_buffer_binding has changed.
 */
void
_mesa_update_vao_client_arrays(struct gl_context *ctx,
                               struct gl_vertex_array_object *vao)
{
   GLbitfield64 arrays = vao->NewArrays;

   while (arrays) {
      const int attrib = u_bit_scan64(&arrays);
      struct gl_vertex_array *client_array = &vao->_VertexAttrib[attrib];
      const struct gl_array_attributes *attrib_array =
         &vao->VertexAttrib[attrib];
      const struct gl_vertex_buffer_binding *buffer_binding =
         &vao->BufferBinding[attrib_array->BufferBindingIndex];

      _mesa_update_client_array(ctx, client_array, attrib_array,
                                buffer_binding);
   }
}


bool
_mesa_all_varyings_in_vbos(const struct gl_vertex_array_object *vao)
{
   /* Walk those enabled arrays that have the default vbo attached */
   GLbitfield64 mask = vao->_Enabled & ~vao->VertexAttribBufferMask;

   while (mask) {
      /* Do not use u_bit_scan64 as we can walk multiple
       * attrib arrays at once
       */
      const int i = ffsll(mask) - 1;
      const struct gl_array_attributes *attrib_array =
         &vao->VertexAttrib[i];
      const struct gl_vertex_buffer_binding *buffer_binding =
         &vao->BufferBinding[attrib_array->BufferBindingIndex];

      /* Only enabled arrays shall appear in the _Enabled bitmask */
      assert(attrib_array->Enabled);
      /* We have already masked out vao->VertexAttribBufferMask  */
      assert(!_mesa_is_bufferobj(buffer_binding->BufferObj));

      /* Bail out once we find the first non vbo with a non zero stride */
      if (buffer_binding->Stride != 0)
         return false;

      /* Note that we cannot use the xor variant since the _BoundArray mask
       * may contain array attributes that are bound but not enabled.
       */
      mask &= ~buffer_binding->_BoundArrays;
   }

   return true;
}

bool
_mesa_all_buffers_are_unmapped(const struct gl_vertex_array_object *vao)
{
   /* Walk the enabled arrays that have a vbo attached */
   GLbitfield64 mask = vao->_Enabled & vao->VertexAttribBufferMask;

   while (mask) {
      const int i = ffsll(mask) - 1;
      const struct gl_array_attributes *attrib_array =
         &vao->VertexAttrib[i];
      const struct gl_vertex_buffer_binding *buffer_binding =
         &vao->BufferBinding[attrib_array->BufferBindingIndex];

      /* Only enabled arrays shall appear in the _Enabled bitmask */
      assert(attrib_array->Enabled);
      /* We have already masked with vao->VertexAttribBufferMask  */
      assert(_mesa_is_bufferobj(buffer_binding->BufferObj));

      /* Bail out once we find the first disallowed mapping */
      if (_mesa_check_disallowed_mapping(buffer_binding->BufferObj))
         return false;

      /* We have handled everything that is bound to this buffer_binding. */
      mask &= ~buffer_binding->_BoundArrays;
   }

   return true;
}

/**********************************************************************/
/* API Functions                                                      */
/**********************************************************************/


/**
 * Helper for _mesa_BindVertexArray() and _mesa_BindVertexArrayAPPLE().
 * \param genRequired  specifies behavour when id was not generated with
 *                     glGenVertexArrays().
 */
static void
bind_vertex_array(struct gl_context *ctx, GLuint id, GLboolean genRequired)
{
   struct gl_vertex_array_object * const oldObj = ctx->Array.VAO;
   struct gl_vertex_array_object *newObj = NULL;

   assert(oldObj != NULL);

   if ( oldObj->Name == id )
      return;   /* rebinding the same array object- no change */

   /*
    * Get pointer to new array object (newObj)
    */
   if (id == 0) {
      /* The spec says there is no array object named 0, but we use
       * one internally because it simplifies things.
       */
      newObj = ctx->Array.DefaultVAO;
   }
   else {
      /* non-default array object */
      newObj = _mesa_lookup_vao(ctx, id);
      if (!newObj) {
         if (genRequired) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBindVertexArray(non-gen name)");
            return;
         }

         /* For APPLE version, generate a new array object now */
	 newObj = _mesa_new_vao(ctx, id);
         if (!newObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindVertexArrayAPPLE");
            return;
         }

         save_array_object(ctx, newObj);
      }

      if (!newObj->EverBound) {
         /* The "Interactions with APPLE_vertex_array_object" section of the
          * GL_ARB_vertex_array_object spec says:
          *
          *     "The first bind call, either BindVertexArray or
          *     BindVertexArrayAPPLE, determines the semantic of the object."
          */
         newObj->ARBsemantics = genRequired;
         newObj->EverBound = GL_TRUE;
      }
   }

   if (ctx->Array.DrawMethod == DRAW_ARRAYS) {
      /* The _DrawArrays pointer is pointing at the VAO being unbound and
       * that VAO may be in the process of being deleted. If it's not going
       * to be deleted, this will have no effect, because the pointer needs
       * to be updated by the VBO module anyway.
       *
       * Before the VBO module can update the pointer, we have to set it
       * to NULL for drivers not to set up arrays which are not bound,
       * or to prevent a crash if the VAO being unbound is going to be
       * deleted.
       */
      ctx->Array._DrawArrays = NULL;
      ctx->Array.DrawMethod = DRAW_NONE;
   }

   ctx->NewState |= _NEW_ARRAY;
   _mesa_reference_vao(ctx, &ctx->Array.VAO, newObj);
}


/**
 * ARB version of glBindVertexArray()
 * This function behaves differently from glBindVertexArrayAPPLE() in
 * that this function requires all ids to have been previously generated
 * by glGenVertexArrays[APPLE]().
 */
void GLAPIENTRY
_mesa_BindVertexArray( GLuint id )
{
   GET_CURRENT_CONTEXT(ctx);
   bind_vertex_array(ctx, id, GL_TRUE);
}


/**
 * Bind a new array.
 *
 * \todo
 * The binding could be done more efficiently by comparing the non-NULL
 * pointers in the old and new objects.  The only arrays that are "dirty" are
 * the ones that are non-NULL in either object.
 */
void GLAPIENTRY
_mesa_BindVertexArrayAPPLE( GLuint id )
{
   GET_CURRENT_CONTEXT(ctx);
   bind_vertex_array(ctx, id, GL_FALSE);
}


/**
 * Delete a set of array objects.
 *
 * \param n      Number of array objects to delete.
 * \param ids    Array of \c n array object IDs.
 */
void GLAPIENTRY
_mesa_DeleteVertexArrays(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLsizei i;

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteVertexArray(n)");
      return;
   }

   for (i = 0; i < n; i++) {
      struct gl_vertex_array_object *obj = _mesa_lookup_vao(ctx, ids[i]);

      if ( obj != NULL ) {
	 assert( obj->Name == ids[i] );

	 /* If the array object is currently bound, the spec says "the binding
	  * for that object reverts to zero and the default vertex array
	  * becomes current."
	  */
	 if ( obj == ctx->Array.VAO ) {
	    _mesa_BindVertexArray(0);
	 }

	 /* The ID is immediately freed for re-use */
	 remove_array_object(ctx, obj);

         if (ctx->Array.LastLookedUpVAO == obj)
            _mesa_reference_vao(ctx, &ctx->Array.LastLookedUpVAO, NULL);

         /* Unreference the array object. 
          * If refcount hits zero, the object will be deleted.
          */
         _mesa_reference_vao(ctx, &obj, NULL);
      }
   }
}


/**
 * Generate a set of unique array object IDs and store them in \c arrays.
 * Helper for _mesa_GenVertexArrays[APPLE]() and _mesa_CreateVertexArrays()
 * below.
 *
 * \param n       Number of IDs to generate.
 * \param arrays  Array of \c n locations to store the IDs.
 * \param create  Indicates that the objects should also be created.
 * \param func    The name of the GL entry point.
 */
static void
gen_vertex_arrays(struct gl_context *ctx, GLsizei n, GLuint *arrays,
                  bool create, const char *func)
{
   GLuint first;
   GLint i;

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n < 0)", func);
      return;
   }

   if (!arrays) {
      return;
   }

   first = _mesa_HashFindFreeKeyBlock(ctx->Array.Objects, n);

   /* For the sake of simplicity we create the array objects in both
    * the Gen* and Create* cases.  The only difference is the value of
    * EverBound, which is set to true in the Create* case.
    */
   for (i = 0; i < n; i++) {
      struct gl_vertex_array_object *obj;
      GLuint name = first + i;

      obj = _mesa_new_vao(ctx, name);
      if (!obj) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
         return;
      }
      obj->EverBound = create;
      save_array_object(ctx, obj);
      arrays[i] = first + i;
   }
}


/**
 * ARB version of glGenVertexArrays()
 * All arrays will be required to live in VBOs.
 */
void GLAPIENTRY
_mesa_GenVertexArrays(GLsizei n, GLuint *arrays)
{
   GET_CURRENT_CONTEXT(ctx);
   gen_vertex_arrays(ctx, n, arrays, false, "glGenVertexArrays");
}


/**
 * APPLE version of glGenVertexArraysAPPLE()
 * Arrays may live in VBOs or ordinary memory.
 */
void GLAPIENTRY
_mesa_GenVertexArraysAPPLE(GLsizei n, GLuint *arrays)
{
   GET_CURRENT_CONTEXT(ctx);
   gen_vertex_arrays(ctx, n, arrays, false, "glGenVertexArraysAPPLE");
}


/**
 * ARB_direct_state_access
 * Generates ID's and creates the array objects.
 */
void GLAPIENTRY
_mesa_CreateVertexArrays(GLsizei n, GLuint *arrays)
{
   GET_CURRENT_CONTEXT(ctx);
   gen_vertex_arrays(ctx, n, arrays, true, "glCreateVertexArrays");
}


/**
 * Determine if ID is the name of an array object.
 *
 * \param id  ID of the potential array object.
 * \return  \c GL_TRUE if \c id is the name of a array object,
 *          \c GL_FALSE otherwise.
 */
GLboolean GLAPIENTRY
_mesa_IsVertexArray( GLuint id )
{
   struct gl_vertex_array_object * obj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id == 0)
      return GL_FALSE;

   obj = _mesa_lookup_vao(ctx, id);
   if (obj == NULL)
      return GL_FALSE;

   return obj->EverBound;
}


/**
 * Sets the element array buffer binding of a vertex array object.
 *
 * This is the ARB_direct_state_access equivalent of
 * glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer).
 */
void GLAPIENTRY
_mesa_VertexArrayElementBuffer(GLuint vaobj, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_vertex_array_object *vao;
   struct gl_buffer_object *bufObj;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* The GL_ARB_direct_state_access specification says:
    *
    *    "An INVALID_OPERATION error is generated by VertexArrayElementBuffer
    *     if <vaobj> is not [compatibility profile: zero or] the name of an
    *     existing vertex array object."
    */
   vao =_mesa_lookup_vao_err(ctx, vaobj, "glVertexArrayElementBuffer");
   if (!vao)
      return;

   /* The GL_ARB_direct_state_access specification says:
    *
    *    "An INVALID_OPERATION error is generated if <buffer> is not zero or
    *     the name of an existing buffer object."
    */
   if (buffer != 0)
      bufObj = _mesa_lookup_bufferobj_err(ctx, buffer,
                                          "glVertexArrayElementBuffer");
   else
      bufObj = ctx->Shared->NullBufferObj;

   if (bufObj)
      _mesa_reference_buffer_object(ctx, &vao->IndexBufferObj, bufObj);
}


void GLAPIENTRY
_mesa_GetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_vertex_array_object *vao;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* The GL_ARB_direct_state_access specification says:
    *
    *   "An INVALID_OPERATION error is generated if <vaobj> is not
    *    [compatibility profile: zero or] the name of an existing
    *    vertex array object."
    */
   vao =_mesa_lookup_vao_err(ctx, vaobj, "glGetVertexArrayiv");
   if (!vao)
      return;

   /* The GL_ARB_direct_state_access specification says:
    *
    *   "An INVALID_ENUM error is generated if <pname> is not
    *    ELEMENT_ARRAY_BUFFER_BINDING."
    */
   if (pname != GL_ELEMENT_ARRAY_BUFFER_BINDING) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetVertexArrayiv(pname != "
                  "GL_ELEMENT_ARRAY_BUFFER_BINDING)");
      return;
   }

   param[0] = vao->IndexBufferObj->Name;
}
