// FindRecogDigit.cpp: implementation of the CFindRecogDigit class.
//
//////////////////////////////////////////////////////////////////////
#include <android/log.h>
#include "StdAfx.h"
#include "FindRecogDigit.h"
#include "LineRecogPrint.h"
#include "ImageBase.h"
#include "ImageFilter.h"
#include "Binarization.h"

#include "Rotation.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#define  LOG_TAG    "recogPassport"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int findstr(TCHAR* str,TCHAR* query,int stpos)
{
	int len = lstrlen(str);
	int len1 = lstrlen(query);
	int i,j;
	for(i=stpos;i<len;i++)
	{
		if(len-i<len1) break;
		for(j=0;j<len1;j++)
		{
			if(str[i+j]!=query[j]) break;
		}
		if(j==len1) return i;
	}
	return -1;
}
void ReplaceStr(TCHAR* str,TCHAR orgCd,TCHAR newCd)
{
	int len = lstrlen(str);
	int i,j;
	for(i=0;i<len;i++)
	{
		if(str[i]==orgCd) str[i]=newCd;
	}
	return ;
}
CFindRecogDigit::CFindRecogDigit()
{
	m_fAngle=0;
    m_tmpFaceDib = NULL;
    m_tmpColorDib = NULL;
}

CFindRecogDigit::~CFindRecogDigit()
{
    m_tmpFaceDib = NULL;
    m_tmpColorDib = NULL;
}

BYTE* LinearNormalize(BYTE* pImg,int w,int h,int &ww,int &hh)
{
	int ZOOMSIZE = 1;
	ww = w / ZOOMSIZE;
	hh = h / ZOOMSIZE;
	BYTE* pImg1 = new BYTE[ww*hh];
	memset(pImg1,0,sizeof(BYTE)*ww*hh);
	int i,j,s = 0,ii,jj;
	for(i=0;i<hh-1;i++)
		for(j=0;j<ww-1;j++)
		{
			s = 0;
			for(ii=i*ZOOMSIZE;ii<=i*ZOOMSIZE+ZOOMSIZE-1;ii++)
				for(jj=j*ZOOMSIZE;jj<=j*ZOOMSIZE+ZOOMSIZE-1;jj++)
					s += pImg[ii*w+jj];
			if(s > ZOOMSIZE * ZOOMSIZE / 4)
				pImg1[i*ww+j] = 1;
		}
		return pImg1;
}
void FlipVertical(BYTE* pImg,int w,int h)
{
	int i,j;
	BYTE* pImg1 = new BYTE[w*h];
	memcpy(pImg1,pImg,w*h);
	for(i=0;i<h;i++)
		for(j=0;j<w;j++)
			pImg[i*w+j] = pImg1[(h-1-i)*w+w-1-j];
	delete[] pImg1;
}
int ZOOMSIZE = 6;
BYTE* LinearNormalizeGray(BYTE* pImg,int w,int h,int &ww,int &hh)
{
	
	int maxwh = max(w,h);
	ZOOMSIZE = max(1,maxwh/500);
	ww = w / ZOOMSIZE;
	hh = h / ZOOMSIZE;
	BYTE* pImg1 = new BYTE[ww*hh];
	memset(pImg1,0,sizeof(BYTE)*ww*hh);
	int i,j,s = 0,ii,jj;
	for(i=0;i<hh-1;i++)
		for(j=0;j<ww-1;j++)
		{
			s = 255;
			for(ii=i*ZOOMSIZE;ii<=i*ZOOMSIZE+ZOOMSIZE-1;ii++)
				for(jj=j*ZOOMSIZE;jj<=j*ZOOMSIZE+ZOOMSIZE-1;jj++)
					if(s > pImg[ii*w+jj])
						s	= pImg[ii*w+jj];
			pImg1[i*ww+j] = s;
		}
		return pImg1;
}
void Dilation(BYTE* pImg,int w,int h)
{
	int i,j;
	BYTE* pImg1 = new BYTE[w*h];
	memcpy(pImg1,pImg,w*h);
	for(i=1;i<h-1;i++)
		for(j=1;j<w-1;j++)
		{
			if(pImg1[i*w+j] == 1) continue;
			if(pImg1[(i-1)*w+j] == 1 || pImg1[(i+1)*w+j] == 1
				|| pImg1[i*w+j-1] == 1 || pImg1[i*w+j+1] == 1)
				pImg[i*w+j] = 1;
		}
		delete[] pImg1;
}
void EnhanceCharacter(BYTE* pImg,int w,int h)
{
	int i,j;
	BYTE* pImg1 = new BYTE[w*h];
	memcpy(pImg1,pImg,w*h);
	for(i=1;i<h-1;i++)
		for(j=1;j<w-1;j++)
		{
			if(pImg1[i*w+j] == 1) continue;
			if((pImg1[(i-1)*w+j] == 1 && pImg1[(i+1)*w+j] == 1)
				|| (pImg1[i*w+j-1] == 1 && pImg1[i*w+j+1] == 1))
				pImg[i*w+j] = 1;
		}
		delete[] pImg1;
}
void RemoveOneThickLine(BYTE* pImg,int w,int h)
{
	int i,j;
	BYTE* pImg1 = new BYTE[w*h];
	memcpy(pImg1,pImg,w*h);
	for(i=1;i<h-1;i++)
		for(j=1;j<w-1;j++)
		{
			if(pImg1[i*w+j] == 0) continue;
			if((pImg1[(i-1)*w+j] == 0 && pImg1[(i+1)*w+j] == 0)|| (pImg1[i*w+j-1] == 0 && pImg1[i*w+j+1] == 0))
				pImg[i*w+j] = 0;
		}
		delete[] pImg1;
}
/*bool CFindRecogDigit::Find_Recog(BYTE* p24Dib,TCHAR* szName,TCHAR* szHanzi,bool& bName,TCHAR* PassportNo,bool& bPassPortNo, TCHAR* BirthDay,bool& bBirthDay,TCHAR* sex,bool& bSex,TCHAR* expireday,bool &bExpireday,TCHAR* sType,TCHAR* Country)
{
	szName[0] = 0;
	PassportNo[0]=0;
	BirthDay[0] = 0;
	sex[0]=0;
    sType[0] = 0;
    szHanzi[0] = 0;
    expireday[0] = 0;
	if(Country!=NULL)Country[0] = 0;
	bName =false;
	bPassPortNo = false;
	bBirthDay = false;
	bSex = false;

	CRect rtRes;
	rtRes.left = rtRes.top = rtRes.bottom = rtRes.right = 0;
	
	int w,h;int i,j;
	BYTE* pGrayImg = CImageBase::MakeGrayImg(p24Dib,w,h);
	float fzoom=1500.0f/max(w,h);
	if(fzoom>2.0f)fzoom=2.0f;
	if(fzoom<0.5f)fzoom=0.5f;
	
	m_tmpColorDib = p24Dib;
	m_tmpfzoom = fzoom;

	if(fzoom!=1.0f){
		BYTE* pNewImg = CImageBase::ZoomImg(pGrayImg,w,h,fzoom);
		delete[] pGrayImg;pGrayImg = pNewImg;
	}
	CImageFilter::CorrectBrightForCameraImg(pGrayImg,w,h);
	CImageFilter::MeanFilter(pGrayImg,w,h);
	//CImageIO::SaveImgToFile(_T("d:\\temp\\org.bmp"),pGrayImg,w,h);
	//BYTE* pBinImg = CImageFilter::GetEdgeExtractionImgWindow(pGrayImg,w,h,3);
	int rc = Find_RecogImg(pGrayImg,w,h,szName,szHanzi, bName, PassportNo, bPassPortNo,  BirthDay,bBirthDay,sex,bSex,expireday,bExpireday,sType,ROTATE_NONE,Country);
// 	if(rc<0){
// 		BYTE* pRotImg = CRotation::RotateRight_Img(pGrayImg,w,h);
// 		delete pGrayImg;pGrayImg = pRotImg;
// 		int ww  = h;int hh = w;
// 		w= ww,h = hh;
// 		rc = Find_RecogImg(pGrayImg,w,h,szName,szHanzi, bName, PassportNo, bPassPortNo,  BirthDay,bBirthDay,sex,bSex,expireday,bExpireday,sType,ROTATE_RIGHT,Country);
// 		if(rc<0){
// 			pRotImg = CRotation::RotateRight_Img(pGrayImg,w,h);
// 			delete pGrayImg;pGrayImg = pRotImg;
// 			ww  = h; hh = w;
// 			w= ww,h = hh;
// 			rc = Find_RecogImg(pGrayImg,w,h,szName,szHanzi, bName, PassportNo, bPassPortNo,  BirthDay,bBirthDay,sex,bSex,expireday,bExpireday,sType,ROTATE_180,Country);
// 			if(rc<0){
// 				pRotImg = CRotation::RotateRight_Img(pGrayImg,w,h);
// 				delete pGrayImg;pGrayImg = pRotImg;
// 				ww  = h;hh = w;
// 				w= ww,h = hh;
// 				rc = Find_RecogImg(pGrayImg,w,h,szName,szHanzi, bName, PassportNo, bPassPortNo,  BirthDay,bBirthDay,sex,bSex,expireday,bExpireday,sType,ROTATE_LEFT,Country);
// 			}
// 		}
// 
// 	}
	delete pGrayImg;
	if(rc>=0)return true;
	else return false;
}*/
int CFindRecogDigit::Find_RecogImg_Main(BYTE* pGrayImg,int w,int h,int facepick,TCHAR* lines, TCHAR* passportType, TCHAR* country,TCHAR* surName,TCHAR* givenNames,TCHAR* passportNumber,TCHAR* passportChecksum,TCHAR* nationality, TCHAR* birth,TCHAR* birthChecksum,TCHAR* sex,TCHAR* expirationDate,TCHAR* expirationChecksum,TCHAR* personalNumber,TCHAR* personalNumberChecksum,TCHAR* secondRowChecksum)
{
    lines[0] = passportType[0] = country[0] = surName[0] = givenNames[0] = passportNumber[0] = passportChecksum[0] = nationality[0] = 0;
    birth[0] = birthChecksum[0] = sex[0] = expirationChecksum[0] = expirationDate[0] = personalNumber[0] = personalNumberChecksum[0] = secondRowChecksum[0] = 0;
    BYTE* pBinImg = CBinarization::Binarization_Windows(pGrayImg,w,h,5);
	//BYTE* pBinImg = CImageFilter::GetEdgeExtractionImgWindow(pGrayImg,w,h,4);
    m_RotId = ROTATE_NONE;
    int rc = Find_RecogImg(pGrayImg,pBinImg,w,h,facepick,lines,passportType,country,surName,givenNames,passportNumber,passportChecksum,nationality,birth,birthChecksum,sex,expirationDate,expirationChecksum,personalNumber,personalNumberChecksum,secondRowChecksum);
    if(rc>=-1 && rc<1)
    {
		BYTE* pRotImg = CRotation::Rotate180_Img(pGrayImg,w,h);
		pGrayImg = pRotImg;
		pRotImg = CRotation::Rotate180_Img(pBinImg,w,h);
		m_RotId = ROTATE_180;
		delete[] pBinImg;pBinImg = pRotImg;
		rc = Find_RecogImg(pGrayImg,pBinImg,w,h,facepick,lines,passportType,country,surName,givenNames,passportNumber,passportChecksum,nationality,birth,birthChecksum,sex,expirationDate,expirationChecksum,personalNumber,personalNumberChecksum,secondRowChecksum,ROTATE_RIGHT,false);
		delete[] pGrayImg;
	}
    else if(rc==-2){
     	BYTE* pRotImg = CRotation::RotateRight_Img(pGrayImg,w,h);
     	pGrayImg = pRotImg;
        pRotImg = CRotation::RotateRight_Img(pBinImg,w,h);
        delete[] pBinImg;pBinImg = pRotImg;

     	int ww  = h;
        int hh = w;
     	w= ww,h = hh;
		m_RotId = ROTATE_RIGHT;
    	rc = Find_RecogImg(pGrayImg,pBinImg,w,h,facepick,lines,passportType,country,surName,givenNames,passportNumber,passportChecksum,nationality,birth,birthChecksum,sex,expirationDate,expirationChecksum,personalNumber,personalNumberChecksum,secondRowChecksum,ROTATE_180);
     	if(rc>=-1 && rc<1){
     		pRotImg = CRotation::Rotate180_Img(pGrayImg,w,h);
     		delete[] pGrayImg;pGrayImg = pRotImg;
            pRotImg = CRotation::Rotate180_Img(pBinImg,w,h);
            delete[] pBinImg;pBinImg = pRotImg;
			m_RotId = ROTATE_LEFT;
     		rc = Find_RecogImg(pGrayImg,pBinImg,w,h,facepick,lines,passportType,country,surName,givenNames,passportNumber,passportChecksum,nationality,birth,birthChecksum,sex,expirationDate,expirationChecksum,personalNumber,personalNumberChecksum,secondRowChecksum,ROTATE_LEFT,false);
     	}
        delete[] pGrayImg;
    }
    LOGI("return value: %d",rc);
    delete[] pBinImg;
    return rc;
}
int CFindRecogDigit::Find_RecogImg(BYTE* pGrayImg,BYTE* pBinOrgImg,int w,int h,int facepick,TCHAR* lines, TCHAR* passportType, TCHAR* country,TCHAR* surName,TCHAR* givenNames,TCHAR* passportNumber,TCHAR* passportChecksum,TCHAR* nationality, TCHAR* birth,TCHAR* birthChecksum,TCHAR* sex,TCHAR* expirationDate,TCHAR* expirationChecksum,TCHAR* personalNumber,TCHAR* personalNumberChecksum,TCHAR* secondRowChecksum,int rotID,bool bRotate)
{
	CRect rtRes;
	rtRes.left = rtRes.top = rtRes.bottom = rtRes.right = 0;
    
    
	int i,j,k;
	int CharW,CharH;
	CharW = w/35;
	CharH = CharW*2;
	BYTE* pRotImg = pGrayImg;
	BYTE* pBinImg = pBinOrgImg;

    CRunProc runProc;
    CRunRtAry LineAry;
    runProc.MakeConnectComponentFromImg(pBinOrgImg, w, h, LineAry, CRect(0, 0, w - 1, h - 1));
	if(bRotate) {
        CRunRtAry mainrts;
        runProc.CopyRunRtAry( mainrts,LineAry);
        m_fAngle = 0;
        double fang = 0.0;
        for(i = mainrts.GetSize() - 1; i >= 0; i --)
        {
            CRect rt = mainrts[i]->m_Rect;
            if(rt.Width() > CharW || rt.Height() > CharH || (rt.Width() < 10 && rt.Height() < 10) ||  mainrts[i]->nPixelNum > CharW*CharH/2)
            {
                delete mainrts[i];
                mainrts.RemoveAt(i);
            }
        }
       // runProc.DeleteNoizeRects(mainrts, CSize(8, 8));
       // DeleteLargeRects(mainrts, CSize(CharW, CharH));
        fang = runProc.GetAngleFromRunRtAry(mainrts, w, h);
        runProc.RemoveAllRunRt(mainrts);
        LOGI("angle %f",fang);
        if (fabs(fang) > 0.05) {
            m_fAngle = fang;
            pRotImg = CRotation::Rotate_GrayImg(pGrayImg, w, h, -fang, 255, true);
            pBinImg = CBinarization::Binarization_Windows(pRotImg, w, h, 5);
            runProc.RemoveAllRunRt(LineAry);
            runProc.MakeConnectComponentFromImg(pBinImg, w, h, LineAry, CRect(0, 0, w - 1, h - 1));
        } else
        {
            bRotate = false;
        }

		//pBinImg = CImageFilter::GetEdgeExtractionImgWindow(pRotImg,w,h,4);
	}
    for(i = LineAry.GetSize() - 1; i >= 0; i --)
    {
        CRect rt = LineAry[i]->m_Rect;
        if(rt.Width() > CharW*2 || rt.Height() > CharH*2 || (rt.Width() < 5 && rt.Height() < 10) ||  LineAry[i]->nPixelNum > CharW*CharH*2)
        {
            delete LineAry[i];
            LineAry.RemoveAt(i);
        }
    }
	//runProc.DeleteNoizeRects(LineAry,CSize(5,10));
	//DeleteLargeRects(LineAry,CSize(CharW*2,CharH*2));
	m_CharHeight = CharH = GetRealCharHeight(LineAry,int(CharH*0.4));


	runProc.DeleteNoizeRects(LineAry,CSize(CharH / 3,CharH / 2));
#ifdef _DEBUG
	BYTE* pTemp = runProc.GetImgFromRunRtAry(LineAry,CRect(0,0,w,h));
	CImageIO::SaveImgToFile(_T("d:\\temp\\aftermerge_vert.bmp"),pTemp,w,h,1);
	delete pTemp;
#endif

//	CImageIO::SaveImgToFile(_T("d:\\temp\\bin.bmp"),pBinImg,w,h,1);
	runProc.SortByOption(LineAry,0,LineAry.GetSize(),SORT_CENTER_X);
    if (LineAry.GetSize() > 1500)
    {
        runProc.DeleteNoizeRects(LineAry, CSize(15,15));
    }
	Merge_for_WordDetect(LineAry);
#ifdef _DEBUG
	pTemp = runProc.GetImgFromRunRtAry(LineAry,CRect(0,0,w,h));
	CImageIO::SaveImgToFile(_T("d:\\temp\\aftermerge.bmp"),pTemp,w,h,1);
	delete pTemp;
#endif
	for(i = LineAry.GetSize() - 1; i >= 0; i --)
	{
		CRect rt = LineAry[i]->m_Rect;
		if(rt.Height() > m_CharHeight*4 || rt.Width() < m_CharHeight*20 || LineAry[i]->nAddNum < 26)
		{
			delete LineAry[i];
			LineAry.RemoveAt(i);
		}
	}
	runProc.SortByOption(LineAry,0,LineAry.GetSize(),SORT_CENTER_Y);
	if(LineAry.GetSize() < 1)
	{
		runProc.RemoveAllRunRt(LineAry);
		if(bRotate)
		{
			delete[] pBinImg;
			delete[] pRotImg;
		}
		return -2;
	}

    LOGI("2 lines exist");
	memset(_surname,0,sizeof(TCHAR)*100);
	memset(_givenname,0,sizeof(TCHAR)*100);
	int nLineNum = 1;
	int maxlen = 0;
	j = 0;
	int id1 = -1,id2 = -1,id3=-1;
	maxlen = 0;
	j = 0;
	id1 = -1,id2 = -1;
	for(i = 0; i < LineAry.GetSize() - 1; i ++)
	{
		j = i + 1;
		if(LineAry[j]->m_Rect.top - LineAry[i]->m_Rect.bottom > m_CharHeight * 3) continue;
		int overW = min(LineAry[i]->m_Rect.right,LineAry[j]->m_Rect.right)-max(LineAry[i]->m_Rect.left,LineAry[j]->m_Rect.left);
		if(overW > maxlen)
		{
			maxlen = overW;
			id1 = i; id2 = j;
			nLineNum = 2;
		}
	}
	if(nLineNum==1)
	{
		id1 = LineAry.GetSize()-1;
		id2 = LineAry.GetSize()-1;
	}
	
	if(LineAry.GetSize()>2)
	{
		int offy = LineAry[id2]->m_Rect.CenterPoint().y-LineAry[id1]->m_Rect.CenterPoint().y;
		for(i = 0; i < 2; i ++)
		{
			if(i==0){
				k=id1-1;
				if(k<0) continue;
				if(LineAry[id1]->m_Rect.top - LineAry[k]->m_Rect.bottom > m_CharHeight * 3) continue;
				if(LineAry[id1]->m_Rect.CenterPoint().y-LineAry[k]->m_Rect.CenterPoint().y >offy*1.5) continue;
			}
			if(i==1)
			{
				k=id2+1;
				if(k>=LineAry.GetSize()) continue;
				if(LineAry[k]->m_Rect.top - LineAry[id2]->m_Rect.bottom > m_CharHeight * 3) continue;
				if(LineAry[k]->m_Rect.CenterPoint().y-LineAry[id2]->m_Rect.CenterPoint().y >offy*1.5) continue;
			}
			if(LineAry[id1]->m_Rect.Width() + LineAry[id2]->m_Rect.Width()+ LineAry[k]->m_Rect.Width()> maxlen)
			{
				maxlen = LineAry[id1]->m_Rect.Width() + LineAry[id2]->m_Rect.Width()+ LineAry[k]->m_Rect.Width();
				id3 = k;
			}

		}
		if(id3<id1 && id3!=-1)
		{
			k=id3;id3 = id2;id2 = id1;id1 = k;
		}
		if(id1 != -1 && id2 != -1 && id3 != -1)
		{
			nLineNum = 3;
		}
	}	//Find code lines


    if(bRotate)
    {
        memcpy(pGrayImg,pRotImg,w*h);
        memcpy(pBinOrgImg,pBinImg,w*h);
    }



	CRect rtRes1,TotalRect;
	TotalRect = LineAry[id1]->m_Rect;
	if(nLineNum>1)
		TotalRect.UnionRect(TotalRect,LineAry[id2]->m_Rect);

	BYTE* pLineImg=NULL,*pLineGrayImg=NULL;
	int linew,lineh,mode,mode1;
	double dis;
	rtRes1 = LineAry[id2]->m_Rect;
	rtRes1.left = TotalRect.left;
	rtRes1.right = TotalRect.right;

	bool bcheck=false;
	CRect subRt = CRect(0,0,0,0);
	CRect LineRt[2];
	bool b44Letters = true;
	for (mode=0;mode<2;mode++)
	{
		if(mode==0){
			subRt = LineAry[id2]->m_Rect;
			pLineImg = runProc.GetImgFromRunRt(LineAry[id2],linew,lineh);
            pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,LineAry[id2]->m_Rect);
//			CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin2.bmp"),pLineImg,linew,lineh,1);
			
		}else{
			subRt = rtRes1;
			pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,rtRes1);
			linew = rtRes1.Width();lineh = rtRes1.Height();
			//pLineImg = CBinarization::Binarization_Windows(pLineGrayImg,linew,lineh,10);
			pLineImg = CBinarization::Binarization_DynamicThreshold(pLineGrayImg,linew,lineh,15,2);
//			CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin2.bmp"),pLineImg,linew,lineh,1);
			
		}
		TCHAR str1[100];
		memset(str1,0,sizeof(TCHAR)*100);
		if(pLineImg)
		{
			if(nLineNum==1)
				Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str1,dis,MODE_TD1_LINE1,true);
			else if(nLineNum==2)
				Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str1,dis,MODE_TD2_LINE2,true);
			else if(nLineNum==3)
				Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str1,dis,MODE_TD3_LINE2,true);
			delete[] pLineGrayImg;delete[] pLineImg;pLineImg=NULL;
		}
		if(nLineNum==1)
		{
			bcheck=CheckChecksum(str1,MODE_TD1_LINE1);
			if(bcheck==true){
				int nlen = lstrlen(str1);
				LineRt[0] = m_lineRecog.m_LineRt;
				LineRt[0].OffsetRect(subRt.left,subRt.top);
				lstrcpy(lines,str1);
				memcpy(passportNumber,&str1[5],sizeof(TCHAR)*9);
				passportNumber[9]=0;
				passportChecksum[0] = str1[14];
				passportChecksum[1] = 0;
				memcpy(nationality,&str1[2],sizeof(TCHAR)*3);
				nationality[3] = 0;

				memcpy(expirationDate,&str1[15],sizeof(TCHAR)*6);
				expirationDate[6] = 0;
// 				expirationChecksum[0] = str1[21];
// 				expirationChecksum[1] = 0;

				memcpy(surName,&str1[21],sizeof(TCHAR)*8);
				ReplaceStr(surName,'<',' ');
				if(country!=NULL){
					memcpy(country,&str1[2],sizeof(TCHAR)*3);
					country[3] = 0;
				}
				secondRowChecksum[0] = str1[nlen-1];
				secondRowChecksum[1] = 0;
				break;
			}
		}
		else if(nLineNum==2)
		{
			bcheck=CheckChecksum(str1,MODE_TD2_LINE2);
			if(bcheck==true){
				int nlen = lstrlen(str1);
				if(nlen==36)b44Letters=false;
				LineRt[0] = m_lineRecog.m_LineRt;
				LineRt[0].OffsetRect(subRt.left,subRt.top);
				lstrcpy(lines,str1);
				memcpy(passportNumber,str1,sizeof(TCHAR)*9);
				passportNumber[9]=0;
				passportChecksum[0] = str1[9];
				passportChecksum[1] = 0;
				memcpy(nationality,&str1[10],sizeof(TCHAR)*3);
				nationality[3] = 0;
				if(nationality[0] == 'I' && nationality[1] == 'H' && nationality[2] == 'D')
				{
					nationality[1] = 'N';
					if(b44Letters == false)
					{
						str1[11] = 'N';
						lines[11] = 'N';
					}

				}
				memcpy(birth,&str1[13],sizeof(TCHAR)*6);
				birth[6]=0;
				birthChecksum[0] = str1[19];
				birthChecksum[1] = 0;
				memcpy(sex,&str1[20],sizeof(TCHAR));
				sex[1]=0;
				memcpy(expirationDate,&str1[21],sizeof(TCHAR)*6);
				expirationDate[6] = 0;
				expirationChecksum[0] = str1[27];
				expirationChecksum[1] = 0;
            
				if(nlen==36)
				{
					memcpy(personalNumber,&str1[28],sizeof(TCHAR)*7);
					personalNumberChecksum[0] = str1[35];
					personalNumberChecksum[1] = 0;
				}
				else
				{
					memcpy(personalNumber,&str1[28],sizeof(TCHAR)*14);
					personalNumberChecksum[0] = str1[42];
					personalNumberChecksum[1] = 0;
				}
				personalNumber[14] = 0;
				secondRowChecksum[0] = str1[nlen-1];
				secondRowChecksum[1] = 0;
				break;
			}
		}
		else if(nLineNum==3)
		{
			bcheck=CheckChecksum(str1,MODE_TD3_LINE2);
			if(bcheck==true){
				int nlen = lstrlen(str1);
				//if(nlen==36)b44Letters=true;
				LineRt[0] = m_lineRecog.m_LineRt;
				LineRt[0].OffsetRect(subRt.left,subRt.top);
				lstrcpy(lines,str1);
				memcpy(nationality,&str1[15],sizeof(TCHAR)*3);
				nationality[3] = 0;

				memcpy(birth,&str1[0],sizeof(TCHAR)*6);
				birth[6]=0;
				birthChecksum[0] = str1[6];
				birthChecksum[1] = 0;
				memcpy(sex,&str1[7],sizeof(TCHAR));
				sex[1]=0;
				memcpy(expirationDate,&str1[8],sizeof(TCHAR)*6);
				expirationDate[6] = 0;
				expirationChecksum[0] = str1[14];
				expirationChecksum[1] = 0;

				memcpy(personalNumber,&str1[18],sizeof(TCHAR)*11);
				personalNumberChecksum[0] = str1[28];
				personalNumberChecksum[1] = 0;
				personalNumber[11] = 0;
				if(country!=NULL){
					memcpy(country,&str1[15],sizeof(TCHAR)*3);
					country[3] = 0;
				}
				secondRowChecksum[0] = str1[nlen-1];
				secondRowChecksum[1] = 0;
				break;
			}
		}
	}
	if(bcheck==false) {
		runProc.RemoveAllRunRt(LineAry);
		if(bRotate)
		{
			delete[] pBinImg;
			delete[] pRotImg;
		}
		return -1;
	}
	if(nLineNum==1)
	{
		runProc.RemoveAllRunRt(LineAry);
		if(bRotate)
		{
			delete[] pBinImg;
			delete[] pRotImg;
		}
		return 1;
	}
    LOGI("second line ok");

	bool bcheckname1=false,bcheckname2=false;
	rtRes1 = LineAry[id1]->m_Rect;
	rtRes1.left = TotalRect.left;
	rtRes1.right = TotalRect.right;
//	CString name1=_T(""),name2=_T("");
    TCHAR hanzi[100] = _T("");

	TCHAR str1[100];
	memset(str1,0,sizeof(TCHAR)*100);
	TCHAR str2[100];
	memset(str2,0,sizeof(TCHAR)*100);
	for (mode=0;mode<2;mode++)
	{
		if(mode==0){
			subRt = LineAry[id1]->m_Rect;
			pLineImg = runProc.GetImgFromRunRt(LineAry[id1],linew,lineh);
            pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,LineAry[id1]->m_Rect);
//			CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin1.bmp"),pLineImg,linew,lineh,1);
		}else{
			subRt = rtRes1;
			pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,rtRes1);
			linew = rtRes1.Width();lineh = rtRes1.Height();
			//pLineImg = CBinarization::Binarization_Windows(pLineGrayImg,linew,lineh,10);
			pLineImg = CBinarization::Binarization_DynamicThreshold(pLineGrayImg,linew,lineh,15,2);
//			CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin1.bmp"),pLineImg,linew,lineh,1);
			
		}
		if(pLineImg)
		{
			TCHAR tempstr[100];
			memset(tempstr,0,sizeof(TCHAR)*100);
			if(nLineNum==2)
			{
				if(b44Letters)
					Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,tempstr,dis,MODE_TD2_44_LINE1,true);
				else
					Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,tempstr,dis,MODE_TD2_36_LINE1,true);
			}
			else if(nLineNum==3)
				Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,tempstr,dis,MODE_TD3_LINE1,true);
			if(nLineNum==2)
			{
				if(m_bFinalCheck==false && b44Letters==true && tempstr[0]=='P') continue;
			}
			if(nLineNum==3)
			{
				if(CheckChecksum(tempstr,MODE_TD3_LINE1)==true)
				{
					lstrcpy(str1,tempstr);
					LineRt[1] = m_lineRecog.m_LineRt;
					LineRt[1].OffsetRect(subRt.left,subRt.top);
					TCHAR strTemp[100];
					lstrcpy(strTemp,str1);
					lstrcat(strTemp,_T("\n"));
					lstrcat(strTemp,lines);
					lstrcpy(lines,strTemp);

					bcheckname1 = true;
					memcpy(passportNumber,&str1[5],sizeof(TCHAR)*9);
					passportNumber[9]=0;
					passportChecksum[0] = str1[14];
					passportChecksum[1] = 0;
					if(country!=NULL){
						memcpy(country,&str1[2],sizeof(TCHAR)*3);
						country[3] = 0;
					}

					passportType[0] = str1[0];
					if(str1[1]=='<') passportType[1] = 0;
					else   passportType[1] = str1[1];
					passportType[2] = 0;
					//LOGI("check name ok");
					break;
				}
				continue;
			}

			ExtractionInformationFromFirstLine(tempstr);
            if(tempstr[2] == 'I' && tempstr[3] == 'H' && tempstr[4] == 'D')
            {
                tempstr[3] = 'N';
            }
			if((lstrcmp(str1,tempstr)==0 || lstrcmp(str2,tempstr)==0) && lstrlen(tempstr) > 0)
			{
					lstrcpy(str1,tempstr);
					LineRt[1] = m_lineRecog.m_LineRt;
					LineRt[1].OffsetRect(subRt.left,subRt.top);
					TCHAR strTemp[100];
					lstrcpy(strTemp,str1);
					lstrcat(strTemp,_T("\n"));
					lstrcat(strTemp,lines);
					lstrcpy(lines,strTemp);

					bcheckname1 = true;
					if(country!=NULL){
						memcpy(country,&str1[2],sizeof(TCHAR)*3);
						country[3] = 0;
					}
					passportType[0] = str1[0];
					if(str1[1]=='<') passportType[1] = 0;
					else   passportType[1] = str1[1];
					passportType[2] = 0;
					//LOGI("check name ok");
					break;
			}
			lstrcpy(str1,tempstr);
			memset(tempstr,0,sizeof(TCHAR)*100);
			if(nLineNum==2)
			{
				if(b44Letters)
					Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,tempstr,dis,MODE_TD2_44_LINE1);
				else
					Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,tempstr,dis,MODE_TD2_36_LINE1);
			}
			if(nLineNum==2)
			{
				if(m_bFinalCheck==false && b44Letters==true && tempstr[0]=='P') continue;
			}

			if(tempstr[2] == 'I' && tempstr[3] == 'H' && tempstr[4] == 'D')
            {
                tempstr[3] = 'N';
            }
			ExtractionInformationFromFirstLine(tempstr);
			delete[] pLineGrayImg;delete[] pLineImg;pLineImg=NULL;
			if((lstrcmp(str1,tempstr)==0 || lstrcmp(str2,tempstr)==0) && lstrlen(tempstr) > 0)
			{
				lstrcpy(str1,tempstr);
				LineRt[1] = m_lineRecog.m_LineRt;
				LineRt[1].OffsetRect(subRt.left,subRt.top);
				TCHAR strTemp[100];
				lstrcpy(strTemp,str1);
				lstrcat(strTemp,_T("\n"));
				lstrcat(strTemp,lines);
				lstrcpy(lines,strTemp);

				bcheckname1 = true;
				if(country!=NULL){
					memcpy(country,&str1[2],sizeof(TCHAR)*3);
					country[3] = 0;
				}
				passportType[0] = str1[0];
				if(str1[1]=='<') passportType[1] = 0;
				else   passportType[1] = str1[1];
				passportType[2] = 0;
				//LOGI("check name ok");
				break;
			}
 
		}	
	}
 	lstrcat(surName,_surname);
	lstrcat(givenNames,_givenname);
	if(bcheckname1==false) {
		runProc.RemoveAllRunRt(LineAry);
		if(bRotate)
		{
			delete[] pBinImg;
			delete[] pRotImg;
		}
		return 0;
	}
	if(nLineNum==3)
	{
		memset(str1,0,sizeof(TCHAR)*100);
		memset(str2,0,sizeof(TCHAR)*100);

		rtRes1 = LineAry[id3]->m_Rect;
		rtRes1.left = min(rtRes1.left,TotalRect.left);
		rtRes1.right = max(rtRes1.right,TotalRect.right);
		for (mode=0;mode<2;mode++)
		{
			if(mode==0){
				subRt = LineAry[id3]->m_Rect;
				pLineImg = runProc.GetImgFromRunRt(LineAry[id3],linew,lineh);
				pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,LineAry[id3]->m_Rect);
				//			CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin1.bmp"),pLineImg,linew,lineh,1);
			}else{
				subRt = rtRes1;
				pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,rtRes1);
				linew = rtRes1.Width();lineh = rtRes1.Height();
				//pLineImg = CBinarization::Binarization_Windows(pLineGrayImg,linew,lineh,10);
				pLineImg = CBinarization::Binarization_DynamicThreshold(pLineGrayImg,linew,lineh,15,2);
				//			CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin1.bmp"),pLineImg,linew,lineh,1);

			}
			if(pLineImg)
			{
				TCHAR tempstr[100];
				memset(tempstr,0,sizeof(TCHAR)*100);
				Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,tempstr,dis,MODE_TD3_LINE3,true);
				ExtractionInformationFromFirstLine(tempstr);
				if((lstrcmp(str1,tempstr)==0 || lstrcmp(str2,tempstr)==0) && lstrlen(tempstr) > 0)
				{
					lstrcpy(str1,tempstr);
					lstrcat(lines,_T("\n"));
					lstrcat(lines,str1);

					bcheckname2 = true;
					break;
				}
				lstrcpy(str1,tempstr);
				memset(tempstr,0,sizeof(TCHAR)*100);
				Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,tempstr,dis,MODE_TD3_LINE3);
				ExtractionInformationFromFirstLine(tempstr);
				//Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str2,dis,MODE_PASSPORT_LINE1,true);
				delete[] pLineGrayImg;delete[] pLineImg;pLineImg=NULL;
				if((lstrcmp(str1,tempstr)==0 || lstrcmp(str2,tempstr)==0) && lstrlen(tempstr) > 0)
				{
					lstrcpy(str1,tempstr);
					lstrcat(lines,_T("\n"));
					lstrcat(lines,str1);
					bcheckname2 = true;
					break;
				}
			}	
		}
		lstrcat(surName,_surname);
		lstrcat(givenNames,_givenname);

	}
	runProc.RemoveAllRunRt(LineAry);
	if(bRotate)
	{
		delete[] pBinImg;
		delete[] pRotImg;
	}
	

	/*if(lstrcmp(country,_T("IND"))==0)
	{
		CRect nameRt;
		int offy = LineRt[0].CenterPoint().y - LineRt[1].CenterPoint().y;

		TCHAR str[100];
		TCHAR str1[100];
		CRunRt* pU;
		nameRt.left = LineRt[1].left + LineRt[1].Width()*288/658;
		nameRt.right = LineRt[1].left + LineRt[1].Width()*482/658;
		nameRt.top = LineRt[1].CenterPoint().y-offy*135/36;
		nameRt.bottom = LineRt[1].CenterPoint().y-offy*100/36;
		int nL = MakeRoughLineAry(pBinImg,w,h,LineAry,nameRt,offy*2/5);
		runProc.SortByOption(LineAry,0,LineAry.GetSize(),SORT_CENTER_Y);
		memset(str,0,sizeof(str));
		int k = 0;
		for(i=0;i<nL;++i){
			pU = LineAry.GetAt(i);
			subRt = pU->m_Rect;
			if(subRt.Width()<40 || subRt.Height()<10 )continue;
			if(subRt.Height()<offy*2/7 )continue;
			memset(str1,0,sizeof(TCHAR)*100);
			j=0;
			for (mode=0;mode<1;mode++)
			{
				subRt = pU->m_Rect;
				if(mode==0){
					pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,subRt);
					linew = subRt.Width();lineh = subRt.Height();
					//pLineImg = CImageFilter::GetEdgeExtractionImgWindow(pGrayImg,w,h,3);
					pLineImg = CBinarization::Binarization_DynamicThreshold(pLineGrayImg,linew,lineh,min(linew/2,6),2);
					//memcpy(pLineGrayImg,pLineImg,linew*lineh);
					CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin2.bmp"),pLineImg,linew,lineh,1);

				}else{
					pLineImg = runProc.GetImgFromRunRt(pU,linew,lineh);
					pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,subRt);
					CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin2.bmp"),pLineImg,linew,lineh,1);
				}

				if(pLineImg)
				{
					if(mode == 0){
						Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str1,dis,MODE_PASSPORT_ENGNAME);
					}
					else{
						TCHAR str2[100];
						double dis2;
						Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str2,dis2,MODE_PASSPORT_ENGNAME);
						if(dis2<dis && lstrlen(str2)>3){
							lstrcpy(str1,str2);
							dis = dis2;
						}
					}
					delete[] pLineGrayImg;delete[] pLineImg;pLineImg=NULL;

				}
			}
			if(dis<2500 && lstrlen(str1)>3){
				nameRt = pU->m_Rect;
				lstrcpy(str,str1);
				k++;
				break;
			}
		}
		placeOfIssue[0] = 0;
		if(k>0){
			lstrcat(placeOfIssue,str);
		}
		runProc.RemoveAllRunRt(LineAry);
		nameRt.left = LineRt[1].left + LineRt[1].Width()*219/658;
		nameRt.right = LineRt[1].left + LineRt[1].Width()*658/658;
		nameRt.top = LineRt[1].CenterPoint().y-offy*182/36;
		nameRt.bottom = LineRt[1].CenterPoint().y-offy*150/36;
		nL = MakeRoughLineAry(pBinImg,w,h,LineAry,nameRt,offy*2/5);
		runProc.SortByOption(LineAry,0,LineAry.GetSize(),SORT_CENTER_Y);
		memset(str,0,sizeof(str));
		k = 0;
		for(i=0;i<nL;++i){
			pU = LineAry.GetAt(i);
			subRt = pU->m_Rect;
			if(subRt.Width()<40 || subRt.Height()<10 )continue;
			if(subRt.Height()<offy*2/7 )continue;
			memset(str1,0,sizeof(TCHAR)*100);
			j=0;
			for (mode=0;mode<1;mode++)
			{
				subRt = pU->m_Rect;
				if(mode==0){
					pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,subRt);
					linew = subRt.Width();lineh = subRt.Height();
					//pLineImg = CImageFilter::GetEdgeExtractionImgWindow(pGrayImg,w,h,3);
					pLineImg = CBinarization::Binarization_DynamicThreshold(pLineGrayImg,linew,lineh,min(linew/2,6),2);
					//memcpy(pLineGrayImg,pLineImg,linew*lineh);
					CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin2.bmp"),pLineImg,linew,lineh,1);

				}else{
					pLineImg = runProc.GetImgFromRunRt(pU,linew,lineh);
					pLineGrayImg = CImageBase::CropImg(pRotImg,w,h,subRt);
					CImageIO::SaveImgToFile(_T("d:\\temp\\cropbin2.bmp"),pLineImg,linew,lineh,1);
				}

				if(pLineImg)
				{
					if(mode == 0){
						Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str1,dis,MODE_PASSPORT_ENGNAME);
					}
					else{
						TCHAR str2[100];
						double dis2;
						Recog_Filter(pLineImg,pLineGrayImg,linew,lineh,str2,dis2,MODE_PASSPORT_ENGNAME);
						if(dis2<dis && lstrlen(str2)>3){
							lstrcpy(str1,str2);
							dis = dis2;
						}
					}
					delete[] pLineGrayImg;delete[] pLineImg;pLineImg=NULL;

				}
			}
			if(dis<2500 && lstrlen(str1)>3){
				nameRt = pU->m_Rect;
				lstrcpy(str,str1);
				k++;
				break;
			}
		}
		placeOfBirth[0] = 0;
		if(k>0){
			lstrcat(placeOfBirth,str);
		}
		runProc.RemoveAllRunRt(LineAry);
	}

*/

	if (facepick == 1)
	{
        CRect faceRt;
        if(nLineNum == 1)
        {
            float expand = LineRt[0].Width() / 10;
            faceRt.left = LineRt[0].left - expand;
            faceRt.top = LineRt[0].CenterPoint().y - LineRt[0].Width()*270/640 - expand;
            faceRt.right = LineRt[0].left+ LineRt[0].Width()*160/640 + expand;
            faceRt.bottom = LineRt[0].CenterPoint().y - LineRt[0].Width()*57/640 + expand;
        }
		else
        {
            float expand = LineRt[1].Width() / 10;
            faceRt.left = LineRt[1].left - expand;
            faceRt.top = LineRt[1].CenterPoint().y - LineRt[1].Width()*210/402 - expand;
            faceRt.right = LineRt[1].left+ LineRt[1].Width()*130/402 + expand;
            faceRt.bottom = LineRt[1].CenterPoint().y - LineRt[1].Width()*80/402 + expand;
        }


		m_tmpFaceDib = MakeOutDib(m_fAngle,faceRt,m_RotId);
	}

	return 1;
}
int CFindRecogDigit::MakeRoughLineAry(BYTE* pBinImg,int w,int h,CRunRtAry& LineAry,CRect subRect,int CharH)
{
	CRunProc runProc;
	int CharW;
	CharW = CharH;


	runProc.MakeConnectComponentFromImg(pBinImg,w,h,LineAry,subRect);
	runProc.DeleteNoizeRects(LineAry,CSize(5,5));
	DeleteLargeRects(LineAry,CSize(CharW*2,CharH*2));
	m_CharHeight = GetRealCharHeight(LineAry,int(CharH*0.4));
	if(m_CharHeight<CharH*0.3)m_CharHeight = CharH;
	CharH = m_CharHeight;
	runProc.SortByOption(LineAry,0,LineAry.GetSize(),SORT_CENTER_X);

	runProc.DeleteNoizeRects(LineAry,CSize(CharH / 3,CharH / 2));
#ifdef _DEBUG
	BYTE* pTemp = runProc.GetImgFromRunRtAry(LineAry,CRect(0,0,w,h));
	CImageIO::SaveImgToFile(_T("d:\\temp\\name.bmp"),pTemp,w,h,1);
	delete pTemp;
#endif

	runProc.SortByOption(LineAry,0,LineAry.GetSize(),SORT_CENTER_X);
	Merge_for_WordDetect(LineAry);
	int nL = LineAry.GetSize();
	return nL;
}
BYTE* CFindRecogDigit::MakeOutDib(double fAngle,CRect subRt,int rotID)
{
	BYTE* newDib,*rotDib,*outDib;
// 	if(m_tmpfzoom != 1.0f)	newDib = CImageBase::ZoomDib(m_tmpColorDib,m_tmpfzoom);
// 	else					newDib = CImageBase::CopyDib(m_tmpColorDib);
	newDib = CImageBase::CopyDib(m_tmpColorDib);
	int w,h;
	CImageBase::GetWidthHeight(newDib,w,h);
	if(rotID != ROTATE_NONE){
		rotDib = CRotation::RotateRegularDib(newDib,rotID);
		delete[] newDib;
		newDib = rotDib;
	}
	if (fabs(fAngle)>0.05){
		rotDib = CRotation::RotateDib(newDib,-fAngle,BACKGROUND_CALC,true);
		delete[] newDib;
		newDib = rotDib;
	}
	//CImageIO::SaveImageToFile(_T("D:\\temp\\color.bmp"),newDib);
	CImageBase::GetWidthHeight(newDib,w,h);
	//subRt.left	= (int)(subRt.left*m_tmpfzoom);
	//subRt.top	= (int)(subRt.top*m_tmpfzoom);
	//subRt.right	= (int)(subRt.right*m_tmpfzoom);
	//subRt.bottom = (int)(subRt.bottom*m_tmpfzoom);
	CRect r = subRt;
// 	r.left = max(0,subRt.left-subRt.Height());
// 	r.right = min(w-1,subRt.right+subRt.Height());
// 	r.top = max(0,subRt.bottom-subRt.Width()*750/1000);
// 	r.bottom = min(h-1,subRt.bottom+subRt.Height());
	
	outDib = CImageBase::CropDib(newDib,r);
	//CImageIO::SaveImageToFile(_T("D:\\temp\\colorCrop.bmp"),outDib);
	delete[] newDib;
	return outDib;
}
float CFindRecogDigit::GetAngleFromImg(BYTE* pImg,int w,int h)
{
	double fang=0.0;
	CRunProc RunProc;
	RunProc.RemoveAllRunRt(mainrts);
	RunProc.MakeConnectComponentFromImg(pImg,w,h,mainrts,CRect(0,0,w-1,h-1));
	int CharW,CharH;
	CharW = w/50;
	CharH = CharW*2;
	RunProc.DeleteNoizeRects(mainrts,CSize(10,10));
	DeleteLargeRects(mainrts,CSize(CharW,CharH));
	fang = RunProc.GetAngleFromRunRtAry(mainrts,w,h);
	RunProc.RemoveAllRunRt(mainrts);
	return (float)fang;
}
float CFindRecogDigit::GetAngleFromImg_1(BYTE* pImg,int w,int h)
{
	double fang=0.0;
	CRunProc RunProc;
	RunProc.RemoveAllRunRt(mainrts);
	RunProc.MakeConnectComponentFromImg(pImg,w,h,mainrts,CRect(0,0,w-1,h-1));
	RunProc.DeleteNoizeRects(mainrts,CSize(5,5));
	fang = RunProc.GetAngleFromRunRtAry_1(mainrts,w,h);
	RunProc.RemoveAllRunRt(mainrts);
	return (float)fang;
}
int CFindRecogDigit::DeleteLargeRects(CRunRtAry& RectAry,CSize Sz)
{
	bool b;
	int wd,hi;

	CRunRt* pU;
	int i,num = RectAry.GetSize();

	for(i=num-1; i>=0; i--){
		b = true;
		pU = RectAry.GetAt(i);
		wd = pU->m_Rect.Width(); hi = pU->m_Rect.Height();
		if(wd >Sz.cx || hi >Sz.cy ){
			b = false;//Small rects 
		}
		else if(pU->nPixelNum > Sz.cx*Sz.cy/2){
			b = false;//Too Small Rects
		}
		if(b == false || pU->bUse == false){
			delete (CRunRt*)RectAry.GetAt(i);
			RectAry.RemoveAt(i);
		}
	}
	num = RectAry.GetSize();
	return num;
}
void CFindRecogDigit::RemoveRectsOutofSubRect(CRunRtAry& RectAry,CRect SubRt)
{
	CRect r;
	CRunRt* pU;
	int i,num = RectAry.GetSize();
	for(i=0;i<num;++i){
		pU= RectAry.GetAt(i);
		r = pU->m_Rect;
		pU->bUse = false;
		if(r.left < SubRt.left)continue;
		if(r.right > SubRt.right)continue;
		if(r.top < SubRt.top)continue;
		if(r.bottom > SubRt.bottom)continue;
		pU->bUse = true;
	}
	DeleteNoneUseRects(RectAry);
}
void CFindRecogDigit::DeleteNoneUseRects(CRunRtAry& RectAry)
{
	CRunRt* pU;
	int i,num = RectAry.GetSize();
	for(i=0;i<num;++i){
		pU = RectAry.GetAt(i);
		if(pU->bUse == false ){
			delete (CRunRt*)RectAry.GetAt(i);
			RectAry.RemoveAt(i);
			i--; num--;
		}
	}
}

void CFindRecogDigit::Recog_Filter(BYTE* pLineImg,BYTE* pGrayImg,int w,int h,TCHAR *str,double &dis,int mode,bool bgray)
{
	
//	CImageIO::SaveImgToFile(_T("d:\\temp\\lineimg.bmp"),pLineImg,w,h,1);
	m_lineRecog.m_bGrayMode = bgray;
	m_lineRecog.LineRecog(pLineImg,pGrayImg,w,h,dis,str,false,mode);
	//if(lstrlen(str) < 5) { str[0] = 0;dis = 99999;}
}

int CFindRecogDigit::GetApproxRowHeight(CCharAry& rts,int w,int h,int& ls) 
{
	int nCharHeight;
	CInsaeRtProc runProc;
	int hist[150],hist1[150];
	memset(hist,0,sizeof(int)*150);
	int i;
	for(i = 0; i < rts.GetSize();i ++)
	{
		int n = rts[i]->m_Rect.Height();
		if(n >= 150) n = 149;
		hist[n] ++;
	}
	memcpy(hist1,hist,sizeof(int)*150);
	for(i=3;i<147;i++)
		hist[i] = (hist1[i-3]+hist1[i-2]+hist1[i-1]+hist1[i]+hist1[i+1]+hist1[i+2]+hist1[i+3])/7;
	nCharHeight = 0;
	int m = 0;
	for(i=10;i<149;i++)
		if(hist[i] > m)
		{
			m = hist[i];
			nCharHeight = i;
		}
  return nCharHeight;
}
inline float* CFindRecogDigit::GetGaussian(int wid)
{
	float *ret = new float[wid+1];
	memset(ret,0,sizeof(float)*(wid+1));
	
	float d = (float)wid/4,m = (float)wid/2;
	float alpha = 1.218754f;
	float sig2 = d*d/(4*alpha);
	
	for (int i=0;i<wid;i++)
		ret[i] = (float)exp(-1*(i-m)*(i-m)/(2*sig2));
	
	return ret;
}

int CFindRecogDigit::GetRealCharHeight(CRunRtAry& RunAry,int minTh)
{

	minTh = max(15,minTh);
	CInsaeRtProc runProc;
	float hist[150],hist1[150];
	memset(hist,0,sizeof(float)*150);
	int i;
	for(i = 0; i < RunAry.GetSize();i ++)
	{
		int n = RunAry[i]->m_Rect.Height();
		if(n >= 150) n = 149;
		if(RunAry[i]->m_Rect.Width() > RunAry[i]->m_Rect.Height()) continue;
		hist[n] ++;
	}
	memcpy(hist1,hist,sizeof(int)*150);
	for(i=minTh;i<147;i++)
		hist[i] = (hist1[i-3]+hist1[i-2]+hist1[i-1]+hist1[i]+hist1[i+1]+hist1[i+2]+hist1[i+3])/7;
	int nCharHeight = 0;
	float m = 0;
	for(i=minTh;i<149;i++)
		if(hist[i] > m)
		{
			m = hist[i];
			nCharHeight = i;
		}
	return nCharHeight;

}
void CFindRecogDigit::Merge_for_Vertical()
{
	int nNum=mainrts.GetSize();
	int nLimit1=max(2,m_CharHeight / 10);
	int nLimit=(int)((m_CharHeight*1.5f));
	
	int i,j,count,nOvlapW,nMinW;
	bool bAppend;CRect rect;
	for(i=0;i<nNum;i++)
	{
		mainrts[i]->bUse=true;
	}
	do {
	count=0;
	for(i=0;i<nNum;i++)
	{
		if(mainrts[i]->bUse==false) continue;
		for(j=0;j<nNum;j++)
		{
			if(i==j || mainrts[j]->bUse==false) continue;
			bAppend=false;
			nOvlapW=min(mainrts[i]->m_Rect.right,mainrts[j]->m_Rect.right)
			 	-max(mainrts[i]->m_Rect.left,mainrts[j]->m_Rect.left);
			if(nOvlapW>0)
			{
			 	nMinW=min(mainrts[i]->m_Rect.Width(),mainrts[j]->m_Rect.Width());
			 	if(float(nOvlapW)/float(nMinW)>0.35f)
			 	{
			 		if(mainrts[i]->m_Rect.top-mainrts[j]->m_Rect.bottom<=nLimit1
			 			&& mainrts[i]->m_Rect.bottom>=mainrts[j]->m_Rect.bottom)
			 			bAppend=true;
			 		else if(mainrts[j]->m_Rect.top-mainrts[i]->m_Rect.bottom<=nLimit1
			 			&& mainrts[j]->m_Rect.bottom>=mainrts[i]->m_Rect.bottom) 
			 			bAppend=true;
			 	}
			}
// 			if(bAppend==false)
// 			{
// 				nOvlapH=min(mainrts[i]->m_Rect.bottom,mainrts[j]->m_Rect.bottom)
// 					-max(mainrts[i]->m_Rect.top,mainrts[j]->m_Rect.top);
// 				nMinH=min(mainrts[i]->UnUse1,mainrts[j]->UnUse1);
// 				if(nOvlapH>0)
// 				{
// 					if(float(nOvlapH)/float(nMinH)>0.8f)
// 					{
// 						if(Get_Distance_between_Rects(i,j)<=nMinH / 2
// 							&& mainrts[i]->m_Rect.right>=mainrts[j]->m_Rect.right)
// 							bAppend=true;
// 						else if(Get_Distance_between_Rects(j,i)<=nMinH / 2
// 							&& mainrts[j]->m_Rect.right>=mainrts[i]->m_Rect.right)
// 							bAppend=true;xsqwcfevfacddnym
// 					}
// 				}
// 			}			
			if(bAppend==true)
			{
				int nMaxW = (mainrts[i]->m_Rect.Height()>mainrts[j]->m_Rect.Height()?mainrts[i]->m_Rect.Width():mainrts[j]->m_Rect.Width());
				rect.UnionRect(mainrts[i]->m_Rect,mainrts[j]->m_Rect);
				if( rect.Height()<nLimit	&& rect.Width() < m_CharHeight)
				{
					mainrts[i]->Append(mainrts[j]);
					mainrts[j]->bUse=false;
					j = -1;
					count++;
				}
			}
			else
			{
				if((mainrts[i]->m_Rect.PtInRect(mainrts[j]->m_Rect.TopLeft())
					&& mainrts[i]->m_Rect.PtInRect(mainrts[j]->m_Rect.BottomRight()))
					|| (mainrts[j]->m_Rect.PtInRect(mainrts[i]->m_Rect.TopLeft())
					&& mainrts[j]->m_Rect.PtInRect(mainrts[i]->m_Rect.BottomRight())))
				{
					mainrts[i]->Append(mainrts[j]);
					mainrts[j]->bUse=false;
					j = -1;
					count++;
				}
			}
		}
	} 
	}while(count!=0);
	for(i=0;i<nNum;i++)
	{
		if(mainrts[i]->bUse==false)
		{
			CRunProc::RemoveRunRt(mainrts,i);
			i--;nNum--;
		}
	}
}
void CFindRecogDigit::Merge_for_WordDetect(CRunRtAry& RunAry)
{
	int nNum=RunAry.GetSize();
	int nLimit=(int)((m_CharHeight*2.5f));
	
	int i,j,count,nOvlapH,nMinH;
	bool bAppend;CRect rect;
	for(i=0;i<nNum;i++)
	{
		RunAry[i]->bUse=true;
		RunAry[i]->nAddNum = 1;
	}
//		do {
	count=0;
	for(i=0;i<nNum;i++)
	{
		if(RunAry[i]->bUse==false) continue;
		if(RunAry[i]->m_Rect.Height()<m_CharHeight/4) continue;
		for(j=0;j<nNum;j++)
		{
			if(i==j || RunAry[j]->bUse==false) continue;
			bAppend=false;

			if(bAppend==false)
			{
				nOvlapH=min(RunAry[i]->m_Rect.bottom,RunAry[j]->m_Rect.bottom)
					-max(RunAry[i]->m_Rect.top,RunAry[j]->m_Rect.top);
				nMinH=min(RunAry[i]->m_Rect.Height(),RunAry[j]->m_Rect.Height());
				if(nOvlapH>0)
				{
					if(float(nOvlapH)/float(nMinH)>0.5f && float(nOvlapH)/float(m_CharHeight)>0.3f)
					{
						if(Get_Distance_between_Rects(RunAry,i,j)<=m_CharHeight*2
							&& RunAry[i]->m_Rect.right>=RunAry[j]->m_Rect.right)
							bAppend=true;
						else if(Get_Distance_between_Rects(RunAry,j,i)<=m_CharHeight*2
							&& RunAry[j]->m_Rect.right>=RunAry[i]->m_Rect.right)
							bAppend=true;
					}
				}
			}			
			if(bAppend==true)
			{
				rect.UnionRect(RunAry[i]->m_Rect,RunAry[j]->m_Rect);
				if(/*rect.Width()<nLimit && */rect.Height()<nLimit)
				{
					RunAry[i]->Append(RunAry[j]);
				
					RunAry[j]->bUse=false;
					j = -1;
					count++;
				}
			}
			else
			{
				if((RunAry[i]->m_Rect.PtInRect(RunAry[j]->m_Rect.TopLeft())
					&& RunAry[i]->m_Rect.PtInRect(RunAry[j]->m_Rect.BottomRight()))
					|| (RunAry[j]->m_Rect.PtInRect(RunAry[i]->m_Rect.TopLeft())
					&& RunAry[j]->m_Rect.PtInRect(RunAry[i]->m_Rect.BottomRight())))
				{
					RunAry[i]->Append(RunAry[j]);
					
					RunAry[j]->bUse=false;
					j = -1;
					count++;
				}
			}
		}
	} 
//		}while(count!=0);
	for(i=0;i<nNum;i++)
	{
		if(RunAry[i]->bUse==false)
		{
			CRunProc::RemoveRunRt(RunAry,i);
			i--;nNum--;
		}
	}
}
int	CFindRecogDigit::Get_Distance_between_Rects(CRunRtAry& RunAry,int RtNo1,int RtNo2)
{
	return RunAry[RtNo1]->m_Rect.left-RunAry[RtNo2]->m_Rect.right;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
int CFindRecogDigit::GetCharHeightByHisto(CCharAry &ary,CRect rtBlk)
{
	int i, k,size = ary.GetSize();
	float hist[100],hist1[100];
	float weight = 0.0f;
	memset(hist,0,sizeof(float)*100);
	memset(hist1,0,sizeof(float)*100);
	for(i = 0;i < size; i ++)
	{
		CRect rt = ary[i]->m_Rect;
		int y2 = rtBlk.CenterPoint().y;
		if(rt.top <= y2 && rt.bottom >= y2)
			weight = 1.0f;
		else
			weight = 0.1f;
		if(ary[i]->m_Rect.Height() < 100)
		{
			hist[ary[i]->m_Rect.Height()] += weight;
		}
		else
		{
			hist[99] += weight;
		}
	}
	float* gauss = GetGaussian(31);
	int dd = 15;
	for(i=0; i<100; i++)
		for (k=-dd; k<dd; k++)
			if (i+k>0 && i+k<100)
			{
				hist1[i] += hist[i+k]*gauss[dd+k];
			}
	int ret = 0,maxh = 0;
	for(i=5;i<100;i++)
		if(maxh < hist1[i])
		{
			maxh = hist1[i];
			ret = i;
		}
	delete gauss;
	return ret;
}
bool CFindRecogDigit::CheckChecksum(TCHAR str[],int mode)
{
	if(mode == MODE_TD3_LINE2)
	{
		int nlen = lstrlen(str);
		if(nlen<25) return false;
		if(nlen>30)nlen=30;

		int n[45];
		int i,s;
		int mul[3] = {7,3,1};
		for(i = 0; i < nlen; i ++)
		{
			if(str[i] >='0' && str[i] <='9')
				n[i] = str[i] - '0';
			else if(str[i] == '<')
				n[i] = 0;
			else
				n[i] = str[i] - 'A' + 10;
		}
		s = 0;
		for(i = 0; i < 6; i ++)
			s += mul[i%3] * n[i];
		if(s%10 != n[6])return false;
		if(str[7]!='M'&& str[7]!='F'&& str[7]!='<') return false;

		s = 0;
		for(i = 8; i < 14; i ++)
			s += mul[(i-8)%3] * n[i];
		if(s%10 != n[14])return false;

		// 		s = 0;
		// 		for(i = 21; i < 27; i ++)
		// 			s += mul[(i-21)%3] * n[i];
		// 		if(s%10 != n[27])return false;

		//if(str[10]!='C'|| str[11]!='H' || str[12]!='R') return false;

		int month = (str[2] - '0') * 10 + str[3] - '0';
		if(month > 12 || month<=0) return false;
		int day = (str[4] - '0') * 10 + str[5] - '0';
		if(day > 31 || day<=0) return false;
	}
	if(mode == MODE_TD3_LINE1)
	{
		int nlen = lstrlen(str);
		if(nlen<25) return false;
		if(nlen>30)nlen=30;

		//if(str[0]!='I'|| str[1]!='C') return false;
		int n[45];
		int i,s;
		int mul[3] = {7,3,1};
		for(i = 0; i < nlen; i ++)
		{
			if(str[i] >='0' && str[i] <='9')
				n[i] = str[i] - '0';
			else if(str[i] == '<')
				n[i] = 0;
			else
				n[i] = str[i] - 'A' + 10;
		}
		s = 0;
		for(i = 0; i < 9; i ++)
			s += mul[i%3] * n[i+5];
		if(s%10 != n[14])return false;


	}
	if(mode == MODE_TD1_LINE1)
	{
		int nlen = lstrlen(str);
		if(nlen<25) return false;
		if(nlen>30)nlen=30;

		if(str[0]!='D') return false;
		int n[45];
		int i,s;
		int mul[3] = {7,3,1};
		for(i = 0; i < nlen; i ++)
		{
			if(str[i] >='0' && str[i] <='9')
				n[i] = str[i] - '0';
			else if(str[i] == '<')
				n[i] = 0;
			else
				n[i] = str[i] - 'A' + 10;
		}
		s = 0;
		for(i = 0; i < 14; i ++)
			s += mul[i%3] * n[i];
		if(s%10 != n[14])return false;

		s = 0;
		for(i = 0; i < 29; i ++)
			s += mul[i%3] * n[i];
		if(s%10 != n[29])return false;

		int month = (str[17] - '0') * 10 + str[18] - '0';
		if(month > 12 || month<=0) return false;
		int day = (str[19] - '0') * 10 + str[20] - '0';
		if(day > 31 || day<=0) return false;

	}
	if(mode == MODE_TD2_LINE2)
	{
		m_bFinalCheck=false;
		int nlen = lstrlen(str);
		if(nlen<36) return false;
		if(nlen<40)nlen=36;
		if(nlen>44)nlen=44;
		str[nlen]=0;
		int n[45];
		int i,s;
		int mul[3] = {7,3,1};
		for(i = 0; i < nlen; i ++)
		{
			if(str[i] >='0' && str[i] <='9')
				n[i] = str[i] - '0';
			else if(str[i] == '<')
				n[i] = 0;
			else
				n[i] = str[i] - 'A' + 10;
		}
		s = 0;
		for(i = 0; i < 9; i ++)
			s += mul[i%3] * n[i];
		if(s%10 != n[9])return false;

		s = 0;
		for(i = 13; i < 19; i ++)
			s += mul[(i-13)%3] * n[i];
		if(s%10 != n[19])return false;

		s = 0;
		for(i = 21; i < 27; i ++)
			s += mul[(i-21)%3] * n[i];
		if(s%10 != n[27])return false;

		//if(str[10]!='C'|| str[11]!='H' || str[12]!='R') return false;
		if(str[20]!='M'&& str[20]!='F') return false;

		int month = (str[15] - '0') * 10 + str[16] - '0';
		if(month > 12 || month<=0) return false;
		int day = (str[17] - '0') * 10 + str[18] - '0';
		if(day > 31 || day<=0) return false;
		s = 0;
		if(nlen>36)
		{
			for(i = 28; i < 42; i ++)
				s += mul[(i-28)%3] * n[i];
			if(s%10 != n[42])
				return true;
		}

		s = 0;
		for(i = 0; i < 10; i ++)
			s += mul[i%3] * n[i];

		for(i = 13; i < 20; i ++)
			s += mul[i%3] * n[i];

		for(i = 21; i < nlen-1; i ++)
			s += mul[(i+2)%3] * n[i];

		if(s%10 != n[nlen-1])
			return true;
		m_bFinalCheck=true;
	}

	return true;
}

bool CFindRecogDigit::ExtractionInformationFromFirstLine(TCHAR str[])
{
	
	memset(_surname,0,sizeof(TCHAR)*100);
	memset(_givenname,0,sizeof(TCHAR)*100);
	int len = lstrlen(str);
	if(len < 30) return false;
// 	if(str[0]!='P') return false;
// 	if(str[1]=='<'|| str[1]=='M'||str[1]=='S' || str[1]=='R' || str[1]=='O')
// 	{
// 		if(str[1]=='<')
// 			_passportType[0] = 'P';
// 		else
// 			_passportType[0] = str[1];
// 
// 	}
// 	else return false;
// 	if(str[2]!=Country[0] || str[3]!=Country[1] || str[4]!=Country[2])
// 	{
// 		str[2]=Country[0]; str[3]=Country[1]; str[4]=Country[2];
// 	}

	int i;
	if(len>30)
	{
		int surLen = findstr(str,_T("<<"),4);
		if(surLen > 5)
		{
			memcpy(_surname,&str[5],sizeof(TCHAR)*(surLen-5));
		}
		else return false;
		int nameLen = findstr(str,_T("<<"),surLen+2);
		if(nameLen > surLen+2)
		{
			memcpy(_givenname,&str[surLen+2],sizeof(TCHAR)*(nameLen - surLen-2));
		}
		else
			return false;
	}
	else
	{
		int surLen = findstr(str,_T("<<"),0);
		if(surLen > 1)
		{
			memcpy(_surname,&str[0],sizeof(TCHAR)*(surLen));
		}
		else return false;
		int nameLen = findstr(str,_T("<<"),surLen+2);
		if(nameLen > surLen+2)
		{
			memcpy(_givenname,&str[surLen+2],sizeof(TCHAR)*(nameLen - surLen-2));
		}
		else
			return false;
	}

	ReplaceStr(_surname,'<',' ');
	ReplaceStr(_givenname,'<',' ');
//	_surname.Replace('<',' ');
//	_givenname.Replace('<',' ');
	return true;
}
/*bool CFindRecogDigit::ReCheckName(BYTE* pBinImg,BYTE* pGrayImg,int w,int h,CRunRtAry& RunAry,TCHAR* strHanzi)
{

	CRunProc runProc;
	int ww,hh;
	BYTE* pWordImg;int i;
	TCHAR strWord[1000];double dis;
	CString strGivenName = _givenname;
	strGivenName.Replace(_T(" "),_T(""));
	CString strSurName = _surname;
	strSurName.Replace(_T(" "),_T(""));
    strGivenName = strSurName  + strGivenName;
   
	bool bRes = false;
	int j = 0;CRect rt;
    float mindis = 99999;
    BYTE* pGrayCrop;
	for(i = 0; i < RunAry.GetSize(); i ++)
	{
		rt = RunAry[i]->m_Rect;
		pWordImg = CImageBase::CropImg(pBinImg,w,h,rt);
        pGrayCrop = CImageBase::CropImg(pGrayImg,w,h,rt);
		ww=rt.Width();hh = rt.Height();
		//pWordImg = runProc.GetImgFromRunRt(RunAry[i],ww,hh);
		CImageIO::SaveImgToFile(_T("d:\\temp\\name.bmp"),pWordImg,ww,hh,1);
		if(pWordImg == NULL) continue;
		lstrcpy(strWord,strGivenName);
		Recog_Filter(pWordImg,pGrayCrop,ww,hh,strWord,dis,MODE_PASSPORT_ENGNAME);//ENG NAME
		if(lstrlen(strWord)>3)		
		{
			int k,len =lstrlen(strWord);
			for(k=0;k<len;++k){
				if(strWord[k] == '0') strWord[k] = 'O';
			}
		}
		delete pWordImg;delete pGrayCrop;
		if(lstrcmp(strGivenName,strWord) == 0)
		{
// 			if(strSurName == _T("KIM") || strSurName == _T("LEE") || strSurName == _T("CHOI") || strSurName == _T("PARK")|| strSurName == _T("HAN") || strSurName == _T("JEONG")
// 				 || strSurName == _T("AHN") || strSurName == _T("GANG") || strSurName == _T("KANG")) 
// 			{
// 				bRes = true;break;
// 			}
			for(j = i - 1; j >= 0; j --)
			{
				rt = RunAry[j]->m_Rect;
				pWordImg = CImageBase::CropImg(pBinImg,w,h,rt);
                pGrayCrop = CImageBase::CropImg(pGrayImg,w,h,rt);
				ww=rt.Width();hh = rt.Height();
				//pWordImg = runProc.GetImgFromRunRt(RunAry[j],ww,hh);
				CImageIO::SaveImgToFile(_T("d:\\temp\\name.bmp"),pWordImg,ww,hh,1);
                CImageIO::SaveImgToFile(_T("d:\\temp\\name_gray.bmp"),pGrayCrop,ww,hh,8);
				if(pWordImg == NULL) continue;
				//lstrcpy(strWord,strSurName);
                strWord[0] = 0;
				Recog_Filter(pWordImg,pGrayCrop,ww,hh,strWord,dis,MODE_PASSPORT_CHNAME);//HANZ NAME
				delete pWordImg;delete pGrayCrop;
                if(dis < mindis)
                {
                    mindis = dis;
                    lstrcpy(strHanzi,strWord);
                }
			}
            bRes = true;
			break;
		}
	}

	return bRes;
}*/
