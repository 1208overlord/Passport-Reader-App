#include <jni.h>
#include <string.h>
#include <stdio.h>

#include <android/log.h>
#include <android/bitmap.h>
#include "engine/StdAfx.h"
#include "engine/FindRecogDigit.h"
#include "engine/ImageBase.h"
#include "engine/ImageFilter.h"
#include "engine/Rotation.h"
#include "engine/Security.h"
#include <time.h>
#include <unistd.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#define  LOG_TAG    "recogPassport"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#ifndef eprintf
#define eprintf(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#endif

#define RGB2GRAY(r,g,b) (((b)*117 + (g)*601 + (r)*306) >> 10)

#define RGB565_R(p) ((((p) & 0xF800) >> 11) << 3)
#define RGB565_G(p) ((((p) & 0x7E0 ) >> 5)  << 2)
#define RGB565_B(p) ( ((p) & 0x1F  )        << 3)
#define MAKE_RGB565(r,g,b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

#define RGBA_A(p) (((p) & 0xFF000000) >> 24)
#define RGBA_R(p) (((p) & 0x00FF0000) >> 16)
#define RGBA_G(p) (((p) & 0x0000FF00) >>  8)
#define RGBA_B(p)  ((p) & 0x000000FF)
#define MAKE_RGBA(r,g,b,a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define ROTATE_0	0
#define ROTATE_90	90
#define ROTATE_180	180
#define ROTATE_270	270

#define RET_OK 0
#define RET_FAILED -1

char szLogs[256];
const char * internalStoragePath;
jobject mainContext;

BYTE* MakeGrayImgFromBitmapBits(void* pixels,int& w,int& h,int wstep,int rot);
BYTE* makeRotatedImg(BYTE* data,int& width,int& height,int rot);
BYTE* makeRotatedImg_Crop(BYTE* data,int& width,int& height,int rot,CRect cropRect,int& ww,int& hh);
void Yuv420spRotate(BYTE* des, BYTE* src, int width, int height,int rot,int* wh);
void Yuv420spRotate180(BYTE* des, BYTE* src, int width, int height);
void Yuv420spRotate90(BYTE* des, BYTE* src, int width, int height);
void Yuv420spRotate270(BYTE* des, BYTE* src, int width, int height);
void convYuv420spToRGBInt(BYTE* yuv420sp, int width, int height, int* rgb);
void convYuv420spToRGBByte(BYTE* yuv420sp, int width, int height, BYTE* rgb);
void convYuv420spToRImage(BYTE* yuv420sp, int width, int height, BYTE* rImage);

void SetDibFromBitmapBits(void* pixels,int w,int h,BYTE* colorDib);
void SetGrayImgFromBitmapBits(void* pixels,int w,int h,BYTE* grayImg);

extern "C"
{
	JNIEXPORT jint JNICALL Java_com_passport_scan_RecogEngine_loadDictionary(JNIEnv*  env, jobject  thiz, jobject activity, jbyteArray img_Dic, jint len_Dic,jbyteArray img_Dic1, jint len_Dic1/*,jbyteArray licenseKey*/,jobject assetManager);
	JNIEXPORT jint JNICALL Java_com_passport_scan_RecogEngine_doRecogYuv420p(JNIEnv* env, jobject thiz,jbyteArray yuvData,jint w, jint h, jint facepick,jint rot,jintArray intData,jobject zfaceBitmap);
};

int bLoaded = 0;
CFindRecogDigit digit;
jint JNICALL Java_com_passport_scan_RecogEngine_loadDictionary(JNIEnv*  env, jobject  thiz, jobject activity, jbyteArray img_Dic, jint len_Dic,jbyteArray img_Dic1, jint len_Dic1/*,jbyteArray licenseKey*/,jobject assetManager)
{

    mainContext = activity;

	BYTE* pImg_Dic = (BYTE*)env->GetByteArrayElements(img_Dic, NULL);
	BYTE* pImg_Dic1 = (BYTE*)env->GetByteArrayElements(img_Dic1, NULL);

	BOOL ret;
	ret = digit.m_lineRecog.m_RecogBottomLine1Gray.LoadDicRes(pImg_Dic, len_Dic);
	if(ret == FALSE)
	    return 0;
	ret = digit.m_lineRecog.m_RecogBottomLine1.LoadDicRes(pImg_Dic1, len_Dic1);
	if(ret == FALSE)
	    return 0;
	//ret = digit.m_lineRecog.m_RecogBottomLine2.LoadDicRes(pImg_Dic1, len_Dic1);
	//if(ret == FALSE) return 0;
	char chCodeBotLine1[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ<");
	WORD codebotLine1[30];
	int i;
	for(i = 0; i < 27; i ++)
		codebotLine1[i] = chCodeBotLine1[i];

	digit.m_lineRecog.m_RecogBottomLine1.SetSelCodeIdTable(codebotLine1,27);
	digit.m_lineRecog.m_RecogBottomLine1Gray.SetSelCodeIdTable(codebotLine1,27);

	bLoaded = 1;

	env->ReleaseByteArrayElements(img_Dic, (jbyte*)pImg_Dic, 0);
	env->ReleaseByteArrayElements(img_Dic1, (jbyte*)pImg_Dic1, 0);
//    env->ReleaseByteArrayElements(licenseKey,(jbyte*)lk,0);
	return 1;//ret;
}

typedef struct tagPassportData{
	char lines[100];
	char passportType[100];
	char country[100];
	char surname[100],givenname[100];
	char passportnumber[100],passportchecksum[100];
	char nationality[100];
	char birth[100];
	char birthchecksum[100];
	char sex[100];
	char expirationdate[100],expirationchecksum[100];
	char personalnumber[100],personalnumberchecksum[100];
	char secondrowchecksum[100];
}PassportData;
PassportData a;

void SetBitmapFromDib(JNIEnv* env, jobject thiz, jobject zBitmap,BYTE* pDib);
BYTE* makeRotatedImg(BYTE* data,int& width,int& height,int rot);
BYTE* makeRotatedDib(BYTE* dib,int rot);
jint JNICALL Java_com_passport_scan_RecogEngine_doRecogYuv420p(JNIEnv* env, jobject thiz,jbyteArray yuvData,jint w, jint h, jint facepick,jint rot,jintArray intData,jobject zfaceBitmap)
{
    LOGI("Debug step0");

    jboolean b;
	int ret = 0;
	BYTE * pyuvData = (BYTE*)env->GetByteArrayElements(yuvData, NULL);
	int* pintData = (int*)env->GetIntArrayElements(intData, NULL);
	//LOGI("width:%d height:%d formNo:%d",w,h,g_formNo);
	//if(bLoaded == 0) return 0;

	BYTE* pTempDib = CImageBase::MakeDib(w,h,24);

    BYTE* pBits = CImageBase::Get_lpBits(pTempDib);
	convYuv420spToRGBByte(pyuvData, w,h,pBits);
    digit.m_tmpColorDib = makeRotatedDib(pTempDib,rot);

    delete []pTempDib;

    BYTE* cropImg = CImageBase::MakeGrayImg(digit.m_tmpColorDib,w,h);
//    CImageFilter::CorrectBrightForCameraImg(cropImg, w,h);
    CImageFilter::MeanFilter(cropImg, w,h);

    //LOGI("Cropwidth:%d height:%d,w,h);

    memset(&a,0,sizeof(PassportData));
    LOGI("Debug step1");

    int res = digit.Find_RecogImg_Main(cropImg, w,h, facepick,a.lines,a.passportType,a.country,a.surname,a.givenname,
    		a.passportnumber,a.passportchecksum,a.nationality,a.birth,a.birthchecksum,a.sex,a.expirationdate,a.expirationchecksum,
    		a.personalnumber,a.personalnumberchecksum,a.secondrowchecksum);
    delete[] cropImg;
    int i,len,k=0;
    len = strlen(a.lines); 			pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.lines[i];
    len = strlen(a.passportType); 	pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.passportType[i];
    len = strlen(a.country); 		pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.country[i];
    len = strlen(a.surname); 		pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.surname[i];
    len = strlen(a.givenname); 		pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.givenname[i];
    len = strlen(a.passportnumber); pintData[k++] = len; 	for(i=0;i<len;++i) pintData[k++] = a.passportnumber[i];
    len = strlen(a.passportchecksum); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.passportchecksum[i];
    len = strlen(a.nationality); 	pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.nationality[i];
    len = strlen(a.birth); 			pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.birth[i];
    len = strlen(a.birthchecksum); 	pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.birthchecksum[i];
    len = strlen(a.sex); 			pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.sex[i];
    len = strlen(a.expirationdate); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.expirationdate[i];
    len = strlen(a.expirationchecksum); pintData[k++] = len;for(i=0;i<len;++i) pintData[k++] = a.expirationchecksum[i];
    len = strlen(a.personalnumber); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.personalnumber[i];
    len = strlen(a.personalnumberchecksum); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.personalnumberchecksum[i];
    len = strlen(a.secondrowchecksum); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.secondrowchecksum[i];

    LOGI("Debug step end");
    if (k>20 && res == 1)
    {
//    	if(digit.m_tmpFaceDib != NULL){
//			int normalW = 200;
//			int normalH = 200;
//			BYTE* zoomDib = CImageBase::ZoomDib(digit.m_tmpFaceDib, normalW, normalH);
//			SetBitmapFromDib(env, thiz, zfaceBitmap,zoomDib);
//			delete []zoomDib;
//			delete []digit.m_tmpFaceDib; digit.m_tmpFaceDib = NULL;
//    	}
       	LOGI("recog success");
       	ret = 1;
    }
    else{
       	LOGI("recog failure");
       	ret = 0;
    }
    env->ReleaseByteArrayElements(yuvData, (jbyte*)pyuvData, 0);
	env->ReleaseIntArrayElements(intData,(jint*)pintData,0);
	return ret;
}
BYTE* makeRotatedImg(BYTE* data,int& width,int& height,int rot)
{
	if(rot == 90) {
		BYTE* rotatedData = new BYTE[width*height];
		memcpy(rotatedData,data,width*height);
		return rotatedData;
	}
	int x,y;
	BYTE* rotatedData = new BYTE[width*height];
	if(rot == 0)//CDefines.FR_90)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				rotatedData[x * height + height - y - 1] = data[x + y * width];
			}
		}

		int tmp = width; // Here we are swapping, that's the difference to #11
		width = height;
		height = tmp;
	}
	else if (rot == 270)//CDefines.FR_180)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				rotatedData[(height-y-1) * width + (width - x-1)] = data[x + y * width];
			}
		}
	}
	else if (rot == 180)//CDefines.FR_270)
	{
		for ( y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				rotatedData[(width-x-1) * width + y] = data[x + y * width];
			}
		}
		int tmp = width; // Here we are swapping, that's the difference to #11
		width = height;
		height = tmp;
	}
	return rotatedData;
}
BYTE* makeRotatedDib(BYTE* dib,int rot)
{
    if (rot == 90)
    {
        return CImageBase::CopyDib(dib);
    }
    else if(rot == 0)
    {
        return CRotation::RotateRight_24Dib(dib);
    }
    else if(rot == 270)
    {
        return CRotation::Rotate180_24Dib(dib);
    }
    return CRotation::RotateLeft_24Dib(dib);
}
jint JNICALL Java_com_TestEngine_RecogEngine_doRecogBitmap(JNIEnv* env, jobject thiz,jobject bitmap,int facepick, jintArray intData,jobject zfaceBitmap)
{
	AndroidBitmapInfo  info;
	void*              pixels;
	int                ret;

	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return 0;
	}

	if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) LOGE("Bitmap format is RGBA_8888 !");
	else if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) LOGE("Bitmap format is RGB_565 !");
	else if (info.format == ANDROID_BITMAP_FORMAT_RGBA_4444) LOGE("Bitmap format is RGBA_4444 !");
	else if (info.format == ANDROID_BITMAP_FORMAT_A_8) LOGE("Bitmap format is A_8 !");
	else LOGE("Bitmap format is Format none !");

	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	/* Now fill the values with a nice little plasma */
	int w,h,wstep;
	w = info.width; h = info.height;wstep = info.stride;
	digit.m_tmpColorDib = CImageBase::MakeDib(w,h,24);
	SetDibFromBitmapBits(pixels,w,h,digit.m_tmpColorDib);
	BYTE* cropImg = new BYTE[w*h];
	SetGrayImgFromBitmapBits(pixels,w,h,cropImg);
	LOGI("(w,h,wstep) = (%5d,%5d,%5d) ",w,h,wstep);
	AndroidBitmap_unlockPixels(env, bitmap);

	int* pintData = (int*)env->GetIntArrayElements(intData, NULL);
	//LOGI("width:%d height:%d formNo:%d",w,h,g_formNo);
	//if(bLoaded == 0) return 0;
    CImageFilter::CorrectBrightForCameraImg(cropImg, w,h);
    CImageFilter::MeanFilter(cropImg, w,h);

    memset(&a,0,sizeof(PassportData));
    LOGI("Debug step1");
    int res = digit.Find_RecogImg_Main(cropImg, w,h, facepick,a.lines,a.passportType,a.country,a.surname,a.givenname,
    		a.passportnumber,a.passportchecksum,a.nationality,a.birth,a.birthchecksum,a.sex,a.expirationdate,a.expirationchecksum,
    		a.personalnumber,a.personalnumberchecksum,a.secondrowchecksum);
    delete[] cropImg;
    int i,len,k=0;
    len = strlen(a.lines); 			pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.lines[i];
    len = strlen(a.passportType); 	pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.passportType[i];
    len = strlen(a.country); 		pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.country[i];
    len = strlen(a.surname); 		pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.surname[i];
    len = strlen(a.givenname); 		pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.givenname[i];
    len = strlen(a.passportnumber); pintData[k++] = len; 	for(i=0;i<len;++i) pintData[k++] = a.passportnumber[i];
    len = strlen(a.passportchecksum); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.passportchecksum[i];
    len = strlen(a.nationality); 	pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.nationality[i];
    len = strlen(a.birth); 			pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.birth[i];
    len = strlen(a.birthchecksum); 	pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.birthchecksum[i];
    len = strlen(a.sex); 			pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.sex[i];
    len = strlen(a.expirationdate); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.expirationdate[i];
    len = strlen(a.expirationchecksum); pintData[k++] = len;for(i=0;i<len;++i) pintData[k++] = a.expirationchecksum[i];
    len = strlen(a.personalnumber); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.personalnumber[i];
    len = strlen(a.personalnumberchecksum); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.personalnumberchecksum[i];
    len = strlen(a.secondrowchecksum); pintData[k++] = len;	for(i=0;i<len;++i) pintData[k++] = a.secondrowchecksum[i];
    LOGI("Debug step end");
    if (digit.m_tmpFaceDib != NULL)
    {
    	int normalW = 200;
    	int normalH = 200;
    	BYTE* zoomDib = CImageBase::ZoomDib(digit.m_tmpFaceDib, normalW, normalH);
    	SetBitmapFromDib(env, thiz, zfaceBitmap,zoomDib);
    	delete []zoomDib;
    	delete []digit.m_tmpFaceDib; digit.m_tmpFaceDib = NULL;
    	LOGI("recog success");
    }
    else{
    	LOGI("recog failure");
    }
	env->ReleaseIntArrayElements(intData,(jint*)pintData,0);
	return ret;
}
BYTE* MakeColorImgFromBitmapBits(void* pixels,int w,int h,int wstep)
{
	int i,j;
	int* pImgRGBA = (int*)pixels;
	BYTE* rgbaPixel,r,g,b;
	BYTE* pColor = new BYTE[w*h*3];
	for(i=0;i<h;i++)for(j=0;j<w;j++)
	{
		rgbaPixel = (BYTE*) &pImgRGBA[i*w+j];
		b = rgbaPixel[0];
		g = rgbaPixel[1];
		r = rgbaPixel[2];
		pColor[i*w*3+j*3] = r;//RGB2GRAY(r,g,b);
		pColor[i*w*3+j*3+1] = g;//RGB2GRAY(r,g,b);
		pColor[i*w*3+j*3+2] = b;//RGB2GRAY(r,g,b);
	}
	return pColor;
}
void SetDibFromBitmapBits(void* pixels,int w,int h,BYTE* colorDib)
{
	int i,j,wstep;
	int* pImgRGBA = (int*)pixels;
	BYTE* rgbaPixel,r,g,b;
	BYTE* pBits = CImageBase::Get_lpBits(colorDib);
	wstep = (24*w+31)/32*4;
	for(i=0;i<h;i++)for(j=0;j<w;j++)
	{
		rgbaPixel = (BYTE*) &pImgRGBA[i*w+j];
		b = rgbaPixel[0];
		g = rgbaPixel[1];
		r = rgbaPixel[2];
		pBits[(h-1-i)*wstep+j*3] = r;//RGB2GRAY(r,g,b);
		pBits[(h-1-i)*wstep+j*3+1] = g;//RGB2GRAY(r,g,b);
		pBits[(h-1-i)*wstep+j*3+2] = b;//RGB2GRAY(r,g,b);
	}
}

void SetGrayImgFromBitmapBits(void* pixels,int w,int h,BYTE* grayImg)
{
	int i,j,wstep;
	int* pImgRGBA = (int*)pixels;
	BYTE* rgbaPixel,r,g,b;
	for(i=0;i<h;i++)for(j=0;j<w;j++)
	{
		rgbaPixel = (BYTE*) &pImgRGBA[i*w+j];
		b = rgbaPixel[0];
		g = rgbaPixel[1];
		r = rgbaPixel[2];
		grayImg[i*w+j] = RGB2GRAY(r,g,b);
	}
}
void SetBitmapFromDib(JNIEnv* env, jobject thiz, jobject zBitmap,BYTE* pDib)
{
	JNIEnv J = *env;

	if (zBitmap == NULL) {
		eprintf("bitmap is null\n");
		return;
	}

	// Get bitmap info
	AndroidBitmapInfo info;
	memset(&info, 0, sizeof(info));
	AndroidBitmap_getInfo(env, zBitmap, &info);
	// Check format, only RGB565 & RGBA are supported
	if (info.width <= 0 || info.height <= 0 ||
		(info.format != ANDROID_BITMAP_FORMAT_RGB_565 && info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)) {
		eprintf("invalid bitmap\n");
		//J->ThrowNew(env, J->FindClass(env, "java/io/IOException"), "invalid bitmap");
		return;
	}

	if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) LOGE("Bitmap format is RGBA_8888 !");
	else if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) LOGE("Bitmap format is RGB_565 !");
	else if (info.format == ANDROID_BITMAP_FORMAT_RGBA_4444) LOGE("Bitmap format is RGBA_4444 !");
	else if (info.format == ANDROID_BITMAP_FORMAT_A_8) LOGE("Bitmap format is A_8 !");
	else LOGE("Bitmap format is Format none !");

	// Lock the bitmap to get the buffer
	void * pixels = NULL;
	int res = AndroidBitmap_lockPixels(env, zBitmap, &pixels);
	if (res<0 || pixels == NULL) {
		eprintf("fail to lock bitmap: %d\n", res);
		//J->ThrowNew(env, J->FindClass(env, "java/io/IOException"), "fail to open bitmap");
		return;
	}
	BYTE* pBits = CImageBase::Get_lpBits(pDib);
	int imgw,imgh,wstep;
	CImageBase::GetWidthHeight(pDib,imgw,imgh);
	wstep = (24*imgw+31)/32*4;
	eprintf("Effect: %dx%d, %d\n", info.width, info.height, info.format);
	if(info.width != imgw || info.height != imgh) return;

	int h = info.height;
	int x = 0, y = 0;
	int r,g,b;
	void *pixel;
	// From top to bottom
	for (y = 0; y < info.height; ++y) {
		// From left to right
		for (x = 0; x < info.width; ++x) {
			int a = 0, r = 0, g = 0, b = 0;

			// Get each pixel by format
			if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) {
				pixel = ((uint16_t *)pixels) + y * info.width + x;

			} else {// RGBA
				pixel = ((uint32_t *)pixels) + y * info.width + x;

			}

			// Grayscale
			//int gray = (r * 38 + g * 75 + b * 15) >> 7;

			//int gray = norImg[y*info.width+x];
			b = pBits[(h-1-y)*wstep+x*3];
			g = pBits[(h-1-y)*wstep+x*3+1];
			r = pBits[(h-1-y)*wstep+x*3+2];
			a = 255;
			// Write the pixel back
			if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) {
				*((uint16_t *)pixel) = MAKE_RGB565(r, g, b);
			} else {// RGBA
				*((uint32_t *)pixel) = MAKE_RGBA(r,g,b,a);//(gray, gray, gray, a);
			}
		}
	}
	eprintf("getnorImg is complete\n");

	AndroidBitmap_unlockPixels(env, zBitmap);
}
void Yuv420spRotate(BYTE* des, BYTE* src, int width, int height,int rot,int* wh)
{
	if(rot == 0) {
		int len = width*height*3/2;//src.length;
		for (int i = 0; i < len; i++) {
			des[i] = src[i];
		}
		wh[0] = width; wh[1] = height;
	}
	else if(rot == ROTATE_90) {
		Yuv420spRotate90(des, src, width, height);
		wh[0] = height; wh[1] = width;
	}
	else if(rot == ROTATE_180) {
		Yuv420spRotate180(des, src, width, height);
		wh[0] = width; wh[1] = height;
	}
	else if(rot == ROTATE_270) {
		Yuv420spRotate270(des, src, width, height);
		wh[0] = height; wh[1] = width;
	}
}
void Yuv420spRotate180(BYTE* des, BYTE* src, int width, int height) {
	int n = 0;
	int uh = height >> 1;
	int wh = width * height;
	// copy y
	for (int j = height - 1; j >= 0; j--) {
		for (int i = width - 1; i >= 0; i--) {
			des[n++] = src[width * j + i];
		}
	}

	for (int j = uh - 1; j >= 0; j--) {
		for (int i = width - 1; i > 0; i -= 2) {
			des[n] = src[wh + width * j + i - 1];
			des[n + 1] = src[wh + width * j + i];
			n += 2;
		}
	}
}

void Yuv420spRotate90(BYTE* des, BYTE* src, int width, int height) {
	int wh = width * height;
	int k = 0;
	for (int i = 0; i < width; i++)
	{
		for (int j = height - 1; j >= 0; j--)
		{
			des[k] = src[width * j + i];
			k++;
		}
	}
	for (int i = 0; i < width; i += 2)
	{
		for (int j = height / 2 - 1; j >= 0; j--)
		{
			des[k] = src[wh + width * j + i];
			des[k + 1] = src[wh + width * j + i + 1];
			k += 2;
		}
	}
}



void Yuv420spRotate270(BYTE* des, BYTE* src, int width, int height) {
	int n = 0;
	int uvHeight = height >> 1;
	int wh = width * height;
	// copy y
	for (int j = width - 1; j >= 0; j--) {
		for (int i = 0; i < height; i++) {
			des[n++] = src[width * i + j];
		}
	}

	for (int j = width - 1; j > 0; j -= 2) {
		for (int i = 0; i < uvHeight; i++) {
			des[n++] = src[wh + width * i + j - 1];
			des[n++] = src[wh + width * i + j];
		}
	}
}


void convYuv420spToRGBInt(BYTE* yuv420sp, int width, int height, int* rgb) {

	int frameSize = width * height;

	for (int j = 0, yp = 0; j < height; j++) {
		int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;

		for (int i = 0; i < width; i++, yp++) {
			int y = (0xff & ((int) yuv420sp[yp])) - 16;
			if (y < 0)
				y = 0;
			if ((i & 1) == 0) {
				v = (0xff & yuv420sp[uvp++]) - 128;
				u = (0xff & yuv420sp[uvp++]) - 128;
			}

			int y1192 = 1192 * y;
			int r = (y1192 + 1634 * v);
			int g = (y1192 - 833 * v - 400 * u);
			int b = (y1192 + 2066 * u);

			if (r < 0)
				r = 0;
			else if (r > 262143)
				r = 262143;
			if (g < 0)
				g = 0;
			else if (g > 262143)
				g = 262143;
			if (b < 0)
				b = 0;
			else if (b > 262143)
				b = 262143;

			rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000)
					| ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
		}
	}
}
void convYuv420spToRGBByte(BYTE* yuv420sp, int width, int height, BYTE* rgb)
{
	int frameSize = width * height;
	//int wstep = 3*width;
	int wstep = (24*width+31)/32*4;
	int v1,v2,u1,u2;
	for (int j = 0, yp = 0; j < height; j++) {
		int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;

		for (int i = 0; i < width; i++, yp++) {
			int y = (0xff & ((int) yuv420sp[yp])) - 16;
			if (y < 0)
				y = 0;
			if ((i & 1) == 0) {
				v = (0xff & yuv420sp[uvp++]) - 128;
				u = (0xff & yuv420sp[uvp++]) - 128;
				v1 = 1634 * v;
				v2 = 833 * v;
				u1 = 400 * u;
				u2 = 2066 * u;
			}

			int y1192 = 1192 * y;
			int r = (y1192 + v1);
			int g = (y1192 - v2 - u1);
			int b = (y1192 + u2);

			if (r < 0)
				r = 0;
			else if (r > 262143)
				r = 262143;
			if (g < 0)
				g = 0;
			else if (g > 262143)
				g = 262143;
			if (b < 0)
				b = 0;
			else if (b > 262143)
				b = 262143;

			rgb[(height-1-j)*wstep+i*3+2] = (b>>10);
			rgb[(height-1-j)*wstep+i*3+1] = (g>>10);
			rgb[(height-1-j)*wstep+i*3] = (r>>10);//b;
		}
	}
}

void convYuv420spToRImage(BYTE* yuv420sp, int width, int height, BYTE* rImage)
{
	int frameSize = width * height;
	int wstep = 3*width;
	int v1,v2,u1,u2;

	for (int j = 0, yp = 0; j < height; j++)
	{
		int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;

		for (int i = 0; i < width; i++, yp++)
		{
			int y = (0xff & ((int) yuv420sp[yp])) - 16;
			if (y < 0)
				y = 0;
			if ((i & 1) == 0)
			{
				v = (0xff & yuv420sp[uvp++]) - 128;
				u = (0xff & yuv420sp[uvp++]) - 128;
				v1 = 1634 * v;
				v2 = 833 * v;
				u1 = 400 * u;
				u2 = 2066 * u;
			}

			int y1192 = 1192 * y;
			int r = (y1192 + v1);
			int g = (y1192 - v2 - u1);
			int b = (y1192 + u2);

			if (r < 0)
				r = 0;
			else if (r > 262143)
				r = 262143;
			if (g < 0)
				g = 0;
			else if (g > 262143)
				g = 262143;
			if (b < 0)
				b = 0;
			else if (b > 262143)
				b = 262143;

			rImage[j*width+i] = (r>>10);
		}
	}
}