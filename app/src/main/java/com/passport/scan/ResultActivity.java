package com.passport.scan;


import com.passport.scan.R;
import android.app.Activity;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.TextView;

public class ResultActivity extends Activity {
	public TextView  mTextResult;
	public ImageView mImageFace;
	final static String TAG = "PassportRecognition";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.result_activity);

		mTextResult = (TextView) findViewById(R.id.txtResult);
		mImageFace = (ImageView) findViewById(R.id.face_image);

		String str = RecogEngine.g_recogResult.GetResultString();
		mTextResult.setText(str);
		if(RecogEngine.facepick == 1){
			if (RecogEngine.g_recogResult.faceBitmap != null)
				mImageFace.setImageBitmap(RecogEngine.g_recogResult.faceBitmap);
		}
	}

	@Override
	public void onBackPressed() {
		super.onBackPressed();
	}

}