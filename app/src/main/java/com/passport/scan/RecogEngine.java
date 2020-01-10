package com.passport.scan;


import java.io.IOException;
import java.io.InputStream;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Rect;
import android.util.Log;

public class RecogEngine
{
	static
	{
		System.loadLibrary("recogPassport");
	}
	
	public static RecogResult g_recogResult = new RecogResult();
	public static int facepick = 1;//0:disable-facepick,1:able-facepick
	
	private static final String TAG = "PassportRecog";
	public 			byte[] 	pDic 	= null;
	public			int		pDicLen = 0;
	public 			byte[] 	pDic1 	= null;
	public			int		pDicLen1 = 0;
	public static 	String[] 	assetNames = {"mMQDF_f_Passport_bottom_Gray.dic","mMQDF_f_Passport_bottom.dic"};

	public native int loadDictionary(Context activity, byte[] img_Dic, int len_Dic, byte[] img_Dic1, int len_Dic1,/*, byte[] licenseKey*/AssetManager assets);
	public native int doRecogYuv420p(byte[] yuvdata,int width,int height, int facepick, int rot,int[] intData,Bitmap faceBitmap);
	//public native int doRecogBitmap(Bitmap bitmap, int[] intData,Bitmap faceBitmap);
	public static int[] intData = new int [3000];
	
	public static int 		NOR_W	= 200;//1200;//1006;
	public static int 		NOR_H	= 200;//750;//1451;

	public Context con;
	public RecogEngine() {
	
	}
	public void initEngine(Context context)
	{
		con = context;
		getAssetFile(assetNames[0],assetNames[1]);

	//	String sLicenseKey = "HHEJBFKOLDOADNEAIJFPMPGGDNNAEIFKCNNGDEGJPKCOBMIICGIOIDHEJKIAHEFJIDMIGMFGAHEMBBBNKMFJOILNALFBGKNGIKPKEDLPILDJFCEAEGMFJMIONLLBMIOJJAOCENAJAKCMKJDJNF";
		int ret = loadDictionary(context, pDic,pDicLen,pDic1,pDicLen1/*,sLicenseKey.getBytes()*/,context.getAssets());
		Log.i("recogPassport","loadDictionary: " + ret);
		if(ret == -1)
		{
			AlertDialog.Builder builder1 = new AlertDialog.Builder(context);
			builder1.setMessage("License failed, please contact to TianLong");
			builder1.setCancelable(true);

			builder1.setPositiveButton(
					"OK",
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int id) {
							dialog.cancel();
						}
					});

			/*builder1.setNegativeButton(
					"No",
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int id) {
							dialog.cancel();
						}
					});*/

			AlertDialog alert11 = builder1.create();
			alert11.show();
		}
	}
	
	public int getAssetFile(String fileName,String fileName1)
	{
	
		int size = 0;
		try {
	    	InputStream is = this.con.getResources().getAssets().open(fileName);
	        size = is.available();
	        pDic = new byte[size];
	        pDicLen = size;
	        is.read(pDic);
	        is.close();
		}
		catch (IOException e) 
		{
			e.printStackTrace();
		}
		
		try {
	    	InputStream is = this.con.getResources().getAssets().open(fileName1);
	        size = is.available();
	        pDic1 = new byte[size];
	        pDicLen1 = size;
	        is.read(pDic1);
	        is.close();
		}
		catch (IOException e) 
		{
			e.printStackTrace();
		}
		return size;
	}
	//If fail, empty string.
	public int doRunData(byte[] data,int width,int height,int facepick,int rot,RecogResult result)
	{
				

		result.faceBitmap = null;
		if(facepick == 1){
			result.faceBitmap = Bitmap.createBitmap(NOR_H,NOR_W, Config.ARGB_8888);
		}
		Log.i("RunTime","width:" + String.valueOf(width) + " rotation:" + String.valueOf(rot));
		long startTime = System.currentTimeMillis();
		int ret = doRecogYuv420p(data, width, height, facepick,rot,intData,result.faceBitmap);
		long endTime = System.currentTimeMillis() - startTime;
		Log.i("RunTime","" + endTime);
	    if(ret == 1)
	    {
	    	result.SetResult(intData);
	    }
	    //Log.i(Defines.APP_LOG_TITLE, "Recog failed - " + String.valueOf(ret) + "- "  + String.valueOf(drawResult[0]));
		return ret;
	}
//	public int doRunBitmap(Bitmap bitmap,RecogResult result)
//	{
//				
//		int[] intData = new int [3000];
//		
//		result.faceBitmap = Bitmap.createBitmap(NOR_H,NOR_W, Config.ARGB_8888);
//	    int ret = doRecogBitmap(bitmap, intData,result.faceBitmap);
//	    if(ret == 1)
//	    {
//	    	result.SetResult(intData);
//	    }
//	    //Log.i(Defines.APP_LOG_TITLE, "Recog failed - " + String.valueOf(ret) + "- "  + String.valueOf(drawResult[0]));
//		return ret;
//	}

}
