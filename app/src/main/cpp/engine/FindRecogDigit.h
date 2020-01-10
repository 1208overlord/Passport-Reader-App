// FindRecogDigit.h: interface for the CFindRecogDigit class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FINDRECOGDIGIT_H__DC78FAF4_0ED7_49D1_93EA_4702DB7CB782__INCLUDED_)
#define AFX_FINDRECOGDIGIT_H__DC78FAF4_0ED7_49D1_93EA_4702DB7CB782__INCLUDED_

#include "LineRecogPrint.h"
class CFindRecogDigit  
{
public:
	CFindRecogDigit();
	virtual ~CFindRecogDigit();

public:
	CLineRecogPrint	m_lineRecog;
	TCHAR _surname[100];
	TCHAR _givenname[100];
	
	int m_CharHeight,m_Linespace;
	CRunRtAry mainrts;

	int		m_w,m_h;

	BYTE*	m_tmpColorDib;
    BYTE*   m_tmpFaceDib;
	double	m_tmpfzoom;
	int		m_RotId;
	float   m_fAngle;
	bool	m_bFinalCheck;
public:	
    int Find_RecogImg_Main(BYTE* pGrayImg,int w,int h,int facepick,TCHAR* lines, TCHAR* passportType, TCHAR* country,TCHAR* surName,TCHAR* givenNames,TCHAR* passportNumber,TCHAR* passportChecksum,TCHAR* nationality, TCHAR* birth,TCHAR* birthChecksum,TCHAR* sex,TCHAR* expirationDate,TCHAR* expirationChecksum,TCHAR* personalNumber,TCHAR* personalNumberChecksum,TCHAR* secondRowChecksum);
    int Find_RecogImg(BYTE* pGrayImg,BYTE* pBinOrgImg,int facepick,int w,int h,TCHAR* lines,  TCHAR* passportType, TCHAR* country,TCHAR* surName,TCHAR* givenNames,TCHAR* passportNumber,TCHAR* passportChecksum,TCHAR* nationality, TCHAR* birth,TCHAR* birthChecksum,TCHAR* sex,TCHAR* expirationDate,TCHAR* expirationChecksum,TCHAR* personalNumber,TCHAR* personalNumberChecksum,TCHAR* secondRowChecksum,int rotID = 0,bool bRotate=true);
  
private:
	
	BYTE*	MakeOutDib(double fAngle,CRect subRt,int rotID);

	void	DeleteNoneUseRects(CRunRtAry& RectAry);
	float	GetAngleFromImg(BYTE* pImg,int w,int h);
	float	GetAngleFromImg_1(BYTE* pImg,int w,int h);
	void	Recog_Filter(BYTE* pLineImg,BYTE* pGrayImg,int w,int h,TCHAR *str,double &dis,int mode,bool bgray=false);
	void	DeleteRegions(CCharAry& rts,int rh,int MODE);
	int		GetApproxRowHeight(CCharAry& rts,int w,int h,int& ls) ;
	inline float* GetGaussian(int wid);
	void	Merge_for_WordDetect(CRunRtAry& RunAry);
	void	Merge_for_Vertical();
	int		GetRealCharHeight(CRunRtAry& RunAry,int minTh);
	int		Get_Distance_between_Rects(CRunRtAry& RunAry,int RtNo1,int RtNo2);
	int		GetCharHeightByHisto(CCharAry &ary,CRect rtBlk);
	void	RemoveRectsOutofSubRect(CRunRtAry& RectAry,CRect SubRt);
	int		DeleteLargeRects(CRunRtAry& RectAry,CSize Sz);
	bool	CheckChecksum(TCHAR str[],int mode);
	bool	ExtractionInformationFromFirstLine(TCHAR str[]);
	bool	ReCheckName(BYTE* pBinImg,BYTE* pGrayImg,int w,int h,CRunRtAry& RunAry,TCHAR* strHanzi);
	int		MakeRoughLineAry(BYTE* pBinImg,int w,int h,CRunRtAry& LineAry,CRect subRect,int CharH);
};

#endif // !defined(AFX_FINDRECOGDIGIT_H__DC78FAF4_0ED7_49D1_93EA_4702DB7CB782__INCLUDED_)
