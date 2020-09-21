#ifndef NUGGET_AVB_H
#define NUGGET_AVB_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AVB_NUM_SLOTS = 8,
  AVB_METADATA_MAX_SIZE = 2048,
  AVB_CHUNK_MAX_SIZE = 512,
  AVB_DEVICE_DATA_SIZE = (256 / 8),
  AVB_SIGNATURE_SIZE = 256,
};

/* error codes specific to this application */
enum {

  /* The requested operation can only be performed by the AP bootloader */
  APP_ERROR_AVB_BOOTLOADER = APP_SPECIFIC_ERROR + 0,

  /* The requested operation can only be performed by the HLOS */
  APP_ERROR_AVB_HLOS,

  /*
   * The requested operation is not allowed by the spec, for example trying to
   * lock something that's already locked. This is different from
   * APP_ERROR_BOGUS_ARGS, which is used for out-of-bounds indices, etc..
   */
  APP_ERROR_AVB_DENIED,

  /* Authorization failure for operations that require it */
  APP_ERROR_AVB_AUTHORIZATION,
};

#ifdef __cplusplus
}
#endif

#endif
