/*
 * Copyright (C) 2013 Klaus Reimer (k@ailis.de)
 * See COPYING file for copying conditions
 */

#include "Transfer.h"
#include "DeviceHandle.h"

jobject wrapTransfer(JNIEnv* env, struct libusb_transfer* transfer)
{
    WRAP_POINTER(env, transfer, "Transfer", "transferPointer");
}

struct libusb_transfer* unwrapTransfer(JNIEnv *env, jobject obj)
{
    UNWRAP_POINTER(env, obj, struct libusb_transfer*, "transferPointer");
}

void resetTransfer(JNIEnv* env, jobject obj)
{
    RESET_POINTER(env, obj, "transferPointer");

    // We already have the class from the previous call.
    // Reset all data fields to initial values (usually NULL/zero).
    field = (*env)->GetFieldID(env, cls, "callback", "L"PACKAGE_DIR"/TransferCallback;");
    (*env)->SetObjectField(env, obj, field, NULL);
    field = (*env)->GetFieldID(env, cls, "callbackUserData", "Ljava/lang/Object;");
    (*env)->SetObjectField(env, obj, field, NULL);
    field = (*env)->GetFieldID(env, cls, "buffer", "Ljava/nio/ByteBuffer;");
    (*env)->SetObjectField(env, obj, field, NULL);
    field = (*env)->GetFieldID(env, cls, "maxNumIsoPackets", "I");
    (*env)->SetIntField(env, obj, field, -1);
}

/**
 * void setDevHandle(DeviceHandle)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setDevHandle)
(
    JNIEnv *env, jobject this, jobject handle
)
{
    NOT_NULL(env, handle, return);
    libusb_device_handle *dev_handle = unwrapDeviceHandle(env, handle);
    if (!dev_handle) return;

    unwrapTransfer(env, this)->dev_handle = dev_handle;
}

/**
 * DeviceHandle getDevHandle()
 */
JNIEXPORT jobject JNICALL METHOD_NAME(Transfer, getDevHandle)
(
    JNIEnv *env, jobject this
)
{
    return wrapDeviceHandle(env, unwrapTransfer(env, this)->dev_handle);
}

/**
 * void setFlags(int)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setFlags)
(
    JNIEnv *env, jobject this, jint flags
)
{
    unwrapTransfer(env, this)->flags = flags;
}

/**
 * byte getFlags()
 */
JNIEXPORT jbyte JNICALL METHOD_NAME(Transfer, getFlags)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->flags;
}

/**
 * void setEndpoint(int)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setEndpoint)
(
    JNIEnv *env, jobject this, jint endpoint
)
{
    unwrapTransfer(env, this)->endpoint = endpoint;
}

/**
 * byte getEndpoint()
 */
JNIEXPORT jbyte JNICALL METHOD_NAME(Transfer, getEndpoint)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->endpoint;
}

/**
 * void setType(int)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setType)
(
    JNIEnv *env, jobject this, jint type
)
{
    unwrapTransfer(env, this)->type = type;
}

/**
 * byte getType()
 */
JNIEXPORT jbyte JNICALL METHOD_NAME(Transfer, getType)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->type;
}

/**
 * void setTimeout(int)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setTimeout)
(
    JNIEnv *env, jobject this, jint timeout
)
{
    unwrapTransfer(env, this)->timeout = timeout;
}

/**
 * int getTimeout()
 */
JNIEXPORT jint JNICALL METHOD_NAME(Transfer, getTimeout)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->timeout;
}

/**
 * int getStatus()
 */
JNIEXPORT jint JNICALL METHOD_NAME(Transfer, getStatus)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->status;
}

/**
 * void setLengthNative(int)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setLengthNative)
(
    JNIEnv *env, jobject this, jint length
)
{
    unwrapTransfer(env, this)->length = length;
}

/**
 * int getLength()
 */
JNIEXPORT jint JNICALL METHOD_NAME(Transfer, getLength)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->length;
}

/**
 * int getActualLength()
 */
JNIEXPORT jint JNICALL METHOD_NAME(Transfer, getActualLength)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->actual_length;
}

static void LIBUSB_CALL transferCallback(struct libusb_transfer *transfer) {
    THREAD_BEGIN(env)

    // The saved reference to the Java Transfer object.
    jobject jTransfer = transfer->user_data;

    // Call back into Java.
    jclass cls = (*env)->GetObjectClass(env, jTransfer);
    jmethodID method = (*env)->GetMethodID(env, cls, "transferCallback", "()V");
    (*env)->CallVoidMethod(env, jTransfer, method);

    // Cleanup Java Transfer object too, if requested.
    if (transfer->flags & LIBUSB_TRANSFER_FREE_TRANSFER)
    {
        resetTransfer(env, jTransfer);
    }

    (*env)->DeleteGlobalRef(env, jTransfer);

    THREAD_END
}

/**
 * void setCallbackNative()
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setCallbackNative)
(
    JNIEnv *env, jobject this
)
{
    // Then, set the callback to the appropriate C function and abuse the user_data field
    // to keep a reference to the Java Transfer object we'll call back to later.
    unwrapTransfer(env, this)->callback = &transferCallback;
    unwrapTransfer(env, this)->user_data = this;

    // To ensure the Java Transfer object's reference will still be valid after waiting on
    // completion (for example it might get GC'd because no references in Java are held to
    // it anymore, while the C part is still working fine), we have to make it a global ref.
    (*env)->NewGlobalRef(env, this);
}

/**
 * void unsetCallbackNative()
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, unsetCallbackNative)
(
    JNIEnv *env, jobject this
)
{
    // If the callback was already set, unset it, and remember to delete the global reference again!
    if (unwrapTransfer(env, this)->callback)
    {
        (*env)->DeleteGlobalRef(env, unwrapTransfer(env, this)->user_data);

        unwrapTransfer(env, this)->callback = NULL;
        unwrapTransfer(env, this)->user_data = NULL;
    }
}

// getCallback() is done in Java. As the Java class already keeps that information,
// it's quicker to just get it that way.

// setUserData() and getUserData() are done fully on the Java side. Since the user_data field in
// the libusb_transfer struct is already used to keep a reference to the Transfer object, this
// data has to be kept in Java. That way you also get garbage collection on it for free.

/**
 * void setBufferNative(ByteBuffer)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setBufferNative)
(
    JNIEnv *env, jobject this, jobject buffer
)
{
    NOT_NULL(env, buffer, return);
    DIRECT_BUFFER(env, buffer, buf_ptr, return);

    unwrapTransfer(env, this)->buffer = buf_ptr;
}

// getBuffer() is done in Java. As the Java class already keeps that information,
// it's quicker to just get it that way.

/**
 * void setNumIsoPacketsNative(int)
 */
JNIEXPORT void JNICALL METHOD_NAME(Transfer, setNumIsoPacketsNative)
(
    JNIEnv *env, jobject this, jint numIsoPackets
)
{
    unwrapTransfer(env, this)->num_iso_packets = numIsoPackets;
}

/**
 * int getNumIsoPackets()
 */
JNIEXPORT jint JNICALL METHOD_NAME(Transfer, getNumIsoPackets)
(
    JNIEnv *env, jobject this
)
{
    return unwrapTransfer(env, this)->num_iso_packets;
}
