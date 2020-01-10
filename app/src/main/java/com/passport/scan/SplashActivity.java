package com.passport.scan;


import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.support.v4.app.ActivityCompat;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class SplashActivity extends Activity implements View.OnClickListener {

	final static String TAG = "PassportRecognition";

	Button m_btnStart;
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		setContentView(R.layout.splash_activity);

		m_btnStart = (Button)findViewById(R.id.button);
		m_btnStart.setOnClickListener(this);
		setPermissionsFor6();

	}
	static boolean m_isPermissionAllowed = false;
	public void setPermissionsFor6()
	{
		int currentSDKVersion = Build.VERSION.SDK_INT;

		if(currentSDKVersion >= 23 && m_isPermissionAllowed==false) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                    10);

        }
	}
	@Override
	public void onRequestPermissionsResult(int requestCode,   String permissions[], int[] grantResults)
	{

		// If request is cancelled, the result arrays are empty.
		if(requestCode == 10)
		{
			List<String> str = new ArrayList<String>();
			int i;
			boolean bAllAlowed = true;
			for(i = 0; i < grantResults.length; i ++)
			{
				if(grantResults[i] != PackageManager.PERMISSION_GRANTED)
				{
					str.add(permissions[i]);
					bAllAlowed = false;
					// break;
				}
			}

			if(bAllAlowed == true)
			{
				m_isPermissionAllowed = true;
			}
			else
			{
				finish();
			}
		}
		super.onRequestPermissionsResult(requestCode, permissions, grantResults);
	}
	@Override
	public void onBackPressed() {
		super.onBackPressed();
	}


	@Override
	public void onClick(View v) {

		if (v.getId() == R.id.button)
		{
			Intent intent = new Intent(this, CameraActivity.class);
			startActivity(intent);
		}
	}
}