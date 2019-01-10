#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#include <sys/ioctl.h>
#define DEV_IOCTL_MAGIC 'S'
#define DEV_IOCTL_WRITE _IOW(DEV_IOCTL_MAGIC, 0, int)

#define DEVDRIVER_NAME "/dev/dev_driver"

#include "android/log.h"
#define LOG_TAG "MyTag"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
/*
 * Class:     JavaDriver
 * Method:    driver_open
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_javadriver_JavaDriver_driver_1open
  (JNIEnv *env, jobject this)
  {
      LOGV("Driver open! %s\n",DEVDRIVER_NAME);
      jint fd;
      fd = open(DEVDRIVER_NAME, O_RDWR);
      LOGV("OPEN RESULT : %d\n",fd);
      return fd;
  }

/*
 * Class:     JavaDriver
 * Method:    driver_close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_javadriver_JavaDriver_driver_1close
  (JNIEnv *env, jobject this, jint fd)
  {
    LOGV("Driver close! %s\n",DEVDRIVER_NAME);
    close(fd);
    return 0;
  }

/*
 * Class:     JavaDriver
 * Method:    driver_read
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_javadriver_JavaDriver_driver_1read
  (JNIEnv *env, jobject this, jint fd)
  {
      jint receive;
      LOGV("Driver Read!\n");
      read(fd, &receive, sizeof(int));
      LOGV("Read : %d\n",receive);
      return receive;
  }

/*
 * Class:     JavaDriver
 * Method:    driver_write
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_javadriver_JavaDriver_driver_1write
  (JNIEnv *env, jobject this, jint fd, jint msg)
  {
    LOGV("Driver Write!\n");
    write(fd, (void *)msg, 0);
    return 0;
  }

/*
 * Class:     JavaDriver
 * Method:    driver_ioctl
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_javadriver_JavaDriver_driver_1ioctl
  (JNIEnv *env, jobject this, jint fd, jint msg)
  {
      LOGV("Driver ioctl : %d\n",msg);
      unsigned long send = (unsigned long)msg;
      ioctl(fd, DEV_IOCTL_WRITE, send);
      return 0;
  }
