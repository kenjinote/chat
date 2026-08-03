#include <jni.h>
#include <string>
#include <opus.h>
#include <android/log.h>

#define LOG_TAG "OpusWrapper"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_example_chat_media_AudioEngine_createEncoder(JNIEnv *env, jobject thiz, jint sampleRate, jint channels, jint application) {
    int error;
    OpusEncoder *encoder = opus_encoder_create(sampleRate, channels, application, &error);
    if (error != OPUS_OK) {
        LOGE("Failed to create Opus encoder: %s", opus_strerror(error));
        return 0;
    }
    // Windows client uses 32kbps
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(32000));
    return reinterpret_cast<jlong>(encoder);
}

JNIEXPORT jint JNICALL
Java_com_example_chat_media_AudioEngine_encode(JNIEnv *env, jobject thiz, jlong encoderHandle, jshortArray pcm, jint frameSize, jbyteArray outOpus) {
    OpusEncoder *encoder = reinterpret_cast<OpusEncoder *>(encoderHandle);
    if (!encoder) return -1;

    jshort *pcmData = env->GetShortArrayElements(pcm, nullptr);
    jbyte *outData = env->GetByteArrayElements(outOpus, nullptr);
    jsize outCapacity = env->GetArrayLength(outOpus);

    int encodedBytes = opus_encode(encoder, pcmData, frameSize, reinterpret_cast<unsigned char *>(outData), outCapacity);

    env->ReleaseShortArrayElements(pcm, pcmData, JNI_ABORT);
    env->ReleaseByteArrayElements(outOpus, outData, 0);

    return encodedBytes;
}

JNIEXPORT void JNICALL
Java_com_example_chat_media_AudioEngine_destroyEncoder(JNIEnv *env, jobject thiz, jlong encoderHandle) {
    OpusEncoder *encoder = reinterpret_cast<OpusEncoder *>(encoderHandle);
    if (encoder) {
        opus_encoder_destroy(encoder);
    }
}

JNIEXPORT jlong JNICALL
Java_com_example_chat_media_AudioEngine_createDecoder(JNIEnv *env, jobject thiz, jint sampleRate, jint channels) {
    int error;
    OpusDecoder *decoder = opus_decoder_create(sampleRate, channels, &error);
    if (error != OPUS_OK) {
        LOGE("Failed to create Opus decoder: %s", opus_strerror(error));
        return 0;
    }
    return reinterpret_cast<jlong>(decoder);
}

JNIEXPORT jint JNICALL
Java_com_example_chat_media_AudioEngine_decode(JNIEnv *env, jobject thiz, jlong decoderHandle, jbyteArray opusData, jint len, jshortArray outPcm, jint frameSize) {
    OpusDecoder *decoder = reinterpret_cast<OpusDecoder *>(decoderHandle);
    if (!decoder) return -1;

    unsigned char* inData = nullptr;
    if (opusData != nullptr && len > 0) {
        inData = reinterpret_cast<unsigned char *>(env->GetByteArrayElements(opusData, nullptr));
    }
    
    jshort *pcmData = env->GetShortArrayElements(outPcm, nullptr);

    int decodedSamples = opus_decode(decoder, inData, len, pcmData, frameSize, 0);

    if (opusData != nullptr && len > 0) {
        env->ReleaseByteArrayElements(opusData, reinterpret_cast<jbyte *>(inData), JNI_ABORT);
    }
    env->ReleaseShortArrayElements(outPcm, pcmData, 0);

    return decodedSamples;
}

JNIEXPORT void JNICALL
Java_com_example_chat_media_AudioEngine_destroyDecoder(JNIEnv *env, jobject thiz, jlong decoderHandle) {
    OpusDecoder *decoder = reinterpret_cast<OpusDecoder *>(decoderHandle);
    if (decoder) {
        opus_decoder_destroy(decoder);
    }
}

}
