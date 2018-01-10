//
// Created by Shuyun on 2018/1/10 0010.
//
#include <jni.h>
#include <string>

extern "C"
JNIEXPORT jstring JNICALL
Java_com_shuyun_pupilla_Pupilla_configuration(JNIEnv *env, jobject instance) {

    // TODO

    std::string str = "123";
    return env->NewStringUTF(str.c_str());
}