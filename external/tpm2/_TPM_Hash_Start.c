// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
//
//     This function is called to process a _TPM_Hash_Start() indication.
//
void
_TPM_Hash_Start(
   void
   )
{
   TPM_RC                 result;
   TPMI_DH_OBJECT         handle;

   // If a DRTM sequence object exists, free it up
   if(g_DRTMHandle != TPM_RH_UNASSIGNED)
   {
       ObjectFlush(g_DRTMHandle);
       g_DRTMHandle = TPM_RH_UNASSIGNED;
   }

   // Create an event sequence object and store the handle in global
   // g_DRTMHandle. A TPM_RC_OBJECT_MEMORY error may be returned at this point
   // The null value for the 'auth' parameter will cause the sequence structure to
   // be allocated without being set as present. This keeps the sequence from
   // being left behind if the sequence is terminated early.
   result = ObjectCreateEventSequence(NULL, &g_DRTMHandle);

   // If a free slot was not available, then free up a slot.
   if(result != TPM_RC_SUCCESS)
   {
       // An implementation does not need to have a fixed relationship between
       // slot numbers and handle numbers. To handle the general case, scan for
       // a handle that is assigned and free it for the DRTM sequence.
       // In the reference implementation, the relationship between handles and
       // slots is fixed. So, if the call to ObjectCreateEvenSequence()
       // failed indicating that all slots are occupied, then the first handle we
       // are going to check (TRANSIENT_FIRST) will be occupied. It will be freed
       // so that it can be assigned for use as the DRTM sequence object.
       for(handle = TRANSIENT_FIRST; handle < TRANSIENT_LAST; handle++)
       {
           // try to flush the first object
           if(ObjectIsPresent(handle))
               break;
       }
       // If the first call to find a slot fails but none of the slots is occupied
       // then there's a big problem
       pAssert(handle < TRANSIENT_LAST);

        // Free the slot
        ObjectFlush(handle);

        // Try to create an event sequence object again. This time, we must
        // succeed.
        result = ObjectCreateEventSequence(NULL, &g_DRTMHandle);
        pAssert(result == TPM_RC_SUCCESS);
   }

   return;
}
