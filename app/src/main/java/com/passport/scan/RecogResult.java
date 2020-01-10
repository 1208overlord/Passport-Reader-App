package com.passport.scan;

import android.graphics.Bitmap;
import android.util.Log;


import java.io.File;
import java.util.Vector;


public class RecogResult 
{
	public String lines = "";
	public String passportType = "";
	public String country = "";
	public String surname = "";
	public String givenname = "";
	public String passportnumber = "";
	public String passportchecksum = "";
	public String nationality = "";
	public String birth = "";
	public String birthchecksum = "";
	public String sex = "";
	public String expirationdate = "";
	public String expirationchecksum = "";
	public String personalnumber = "";
	public String personalnumberchecksum = "";
	public String secondrowchecksum = "";

	public String result="";
	
	public Bitmap faceBitmap = null;

	public int getdocType(String lines_str){
		String[] separated = lines_str.split("\n");

		int linecount = separated.length;
		int ch_num= separated[0].length();
		String first_ch=separated[0].substring(0,1);
		if (linecount == 3 && ch_num == 30) {
			return 0;

		}
		else if (linecount == 2 && ch_num == 36) {

			if (first_ch.compareTo("V")==1) {
				return 4;
			}
			else {
				return 1;
			}

		}
		else if (linecount == 2 && ch_num == 44) {
			if (first_ch.compareTo("V")==1) {
				return 3;
			}
			else {
				return 2;
			}

		}
		return -1;
	}
	public String RemoveAlgleBracket(String str) {
		String tmp_str = "";
		boolean find_anglebracket = false;

		for (int i = 0; i < str.length(); i++) {
			if (find_anglebracket&&str.charAt(i) == '<') {
				continue;
			}
			else if (find_anglebracket&&str.charAt(i) != '<') {
				find_anglebracket = false;
				tmp_str += str.charAt(i);
			}

			else if (str.charAt(i) == '<') {
				if (!find_anglebracket) {
					find_anglebracket = true;
					tmp_str+=" ";
				}
			}
			else {
				tmp_str += str.charAt(i);
			}

		}

		return tmp_str;
	}

	public void analysisMRZ(String lines_str, int res){
		String[] separated_lines=lines_str.split("\n");
		if (res == 0) {
			/////////MRZTD1
			String line_tmp = separated_lines[0];										//get first line
			result="DocType: "+ RemoveAlgleBracket(line_tmp.substring(0, 2))+"\n";		//document type
			String country = line_tmp.substring(2, 5);									//country
			result+="Country: "+RemoveAlgleBracket(country)+"\n";
			result +="Document Num: "+RemoveAlgleBracket(line_tmp.substring(5, 14))+"\n";	//document number
			result+="Hash1: "+ line_tmp.substring(14, 15)+"\n";					//hash 1
			result +="OptionalData1: "+RemoveAlgleBracket(line_tmp.substring(15, 30))+"\n";	//OptionalData1
			line_tmp = separated_lines[1];													//get second line
			//String BIRTHDATE = line_tmp.substring(0, 6);									// date of birth
			result +="DateOfBirth: "+line_tmp.substring(0,2)+"/"+line_tmp.substring(2,4)
					+"/"+line_tmp.substring(4,6)+"\n";
			result +="Hash2: "+line_tmp.substring(6, 7)+"\n";								//hash 2
			result +="Gender: "+line_tmp.substring(7, 8)+"\n";								//gender
			result +="ExpiryDate: "+line_tmp.substring(8, 10)+"/"+line_tmp.substring(10,12)
				+"/"+line_tmp.substring(12, 14)+"\n";										//expiry date
			result +="Hash3"+line_tmp.substring(14, 15)+"\n";								//hash 3
			result +="Nationality: "+RemoveAlgleBracket(line_tmp.substring(15, 18))+"\n";							//nationality
			result +="OptionalData2: "+RemoveAlgleBracket(line_tmp.substring(18, 29))+"\n";							//OptionalData2
			result +="Final Hash: "+line_tmp.substring(29, 30)+"\n";						//final hash

			line_tmp = separated_lines[2];													//get third line
			String[] sept_names=line_tmp.split("<<");
			if(sept_names.length>=2){
				result +="Last Name: "+RemoveAlgleBracket(sept_names[0])+"\n";				// last name
				result +="Given name:"+ RemoveAlgleBracket(line_tmp.substring(sept_names[0].length(), 30))+"\n";	// given name
			}else{
				result +="Last Name: \n";
				result +="Given name: "+RemoveAlgleBracket(line_tmp.substring(0, 30));		// given name

			}

		}
		else if (res == 1) {
			String line_tmp = separated_lines[0];											//get first line
			result +="DocType: "+ RemoveAlgleBracket(line_tmp.substring(0, 2))+"\n";		//document type
			String country = line_tmp.substring(2, 5);										//country
			result +="Country: "+RemoveAlgleBracket(country)+"\n";
			String name_line=line_tmp.substring(5, 36);
			String[] sept_names=name_line.split("<<");
			if(sept_names.length>=2){
				result +="Last Name: "+RemoveAlgleBracket(sept_names[0])+"\n";			// last name
				result +="Given name:"+ RemoveAlgleBracket(name_line.substring(sept_names[0].length(), 29))+"\n";	// given name
			}else{
				result +="Last Name: \n";
				result +="Given name: "+RemoveAlgleBracket(line_tmp.substring(5, 36));			// given name

			}
			//result += "Given names: "+RemoveAlgleBracket(line_tmp.substring(5, 36))+"\n";							// given name

			line_tmp = separated_lines[1];													//get second line
			result += "Document Num: "+RemoveAlgleBracket(line_tmp.substring(0, 9))+"\n";									//document number
			result +="Hash1: "+line_tmp.substring(9, 10)+"\n";								//hash 1
			result +="Nationality: "+RemoveAlgleBracket(line_tmp.substring(10, 13))+"\n";							//nationality
			result +="DateOfBirth: "+line_tmp.substring(13,15)+"/"+line_tmp.substring(15,17)
					+"/"+line_tmp.substring(17,19)+"\n";
			result +="Hash2: "+line_tmp.substring(19, 20)+"\n";								//hash 2
			result +="Gender: "+line_tmp.substring(20, 21)+"\n";							//gender
			result +="ExpiryDate: "+line_tmp.substring(21, 23)+"/"+line_tmp.substring(23,25)
					+"/"+line_tmp.substring(25, 27)+"\n";									//expiry date
			result +="Hash3: "+line_tmp.substring(27, 28)+"\n";								//hash 3
			result +="OptionalData: "+RemoveAlgleBracket(line_tmp.substring(28, 35))+"\n";	//OptionalData
			result +="Final hash: "+line_tmp.substring(35, 36)+"\n";						//final hash
		}
		else if (res == 2) {
			/////////MRZTD3
			String line_tmp = separated_lines[0];											//get first line
			result +="DocType: "+ RemoveAlgleBracket(line_tmp.substring(0, 2))+"\n";		//document type
			String country = line_tmp.substring(2, 5);										//country
			result +="Country: "+RemoveAlgleBracket(country)+"\n";
			//result +="Given Names: "+RemoveAlgleBracket(line_tmp.substring(5, 44))+"\n";							// given name
			String name_line=line_tmp.substring(5, 44);
			String[] sept_names=name_line.split("<<");
			if(sept_names.length>=2){
				result +="Last Name: "+RemoveAlgleBracket(sept_names[0])+"\n";			// last name
				result +="Given name:"+ RemoveAlgleBracket(name_line.substring(sept_names[0].length(), 39))+"\n";	// given name
			}else{
				result +="Last Name: \n";
				result +="Given name: "+RemoveAlgleBracket(line_tmp.substring(5, 44));			// given name

			}
			line_tmp = separated_lines[1];													//get second line
			result +="Document Num: "+RemoveAlgleBracket(line_tmp.substring(0, 9))+"\n";									//document number
			result +="Hash1: "+line_tmp.substring(9, 10)+"\n";					//hash 1
			result +="Nationality: "+RemoveAlgleBracket(line_tmp.substring(10, 13))+"\n";							//nationality
			result +="DateOfBirth: "+line_tmp.substring(13,15)+"/"+line_tmp.substring(15,17)
					+"/"+line_tmp.substring(17,19)+"\n";
			result +="Hash2: "+line_tmp.substring(19, 20)+"\n";								//hash 2
			result +="Gender"+line_tmp.substring(20, 21)+"\n";								//gender
			result +="ExpiryDate: "+line_tmp.substring(21, 23)+"/"+line_tmp.substring(23,25)
					+"/"+line_tmp.substring(25, 27)+"\n";									//expiry date
			result +="Hash3: "+line_tmp.substring(27, 28)+"\n";								//hash 3
			result +="PersonalNumber: "+RemoveAlgleBracket(line_tmp.substring(28,42))+"\n";	//Personal Number
			result +="Hash4: "+line_tmp.substring(42, 43)+"\n";								//hash 4
			result +="Final Hash"+line_tmp.substring(43, 44)+"\n";								//final hash
		}
		else if (res == 3) {
			/////////MRV-A
			String line_tmp = separated_lines[0];								//get first line
			result +="DocType: "+ RemoveAlgleBracket(line_tmp.substring(0, 2))+"\n";					//document type
			String country = line_tmp.substring(2, 5);							//country
			result +="Country: "+RemoveAlgleBracket(country)+"\n";
			//result +="Given Names: "+RemoveAlgleBracket(line_tmp.substring(5, 44))+"\n";							// given name
			String name_line=line_tmp.substring(5, 44);
			String[] sept_names=name_line.split("<<");
			if(sept_names.length>=2){
				result +="Last Name: "+RemoveAlgleBracket(sept_names[0])+"\n";			// last name
				result +="Given name:"+ RemoveAlgleBracket(name_line.substring(sept_names[0].length(), 39))+"\n";	// given name
			}else{
				result +="Last Name: \n";
				result +="Given name: "+RemoveAlgleBracket(line_tmp.substring(5, 44));			// given name

			}
			line_tmp = separated_lines[1];											//get second line
			result +="Document Num: "+RemoveAlgleBracket(line_tmp.substring(0, 9))+"\n";									//document number
			result +="Hash1: "+line_tmp.substring(9, 10)+"\n";					//hash 1
			result +="Nationality: "+RemoveAlgleBracket(lines_str.substring(10, 13))+"\n";							//nationality
			result +="DateOfBirth: "+line_tmp.substring(13,15)+"/"+line_tmp.substring(15,17)
					+"/"+line_tmp.substring(17,19)+"\n";
			result +="Hash2: "+line_tmp.substring(19, 20)+"\n";								//hash 2
			result +="Gender"+line_tmp.substring(20, 21)+"\n";								//gender
			result +="ExpiryDate: "+line_tmp.substring(21, 23)+"/"+line_tmp.substring(23,25)
					+"/"+line_tmp.substring(25, 27)+"\n";								//expiry date

			result +="Hash3: "+line_tmp.substring(27, 28)+"\n";								//hash 3

			result +="OptionalData: "+RemoveAlgleBracket(lines_str.substring(28, 44))+"\n";						//OptionalData
		}
		else{
			/////////MRV-B
			String line_tmp = separated_lines[0];								//get first line
			result +="DocType: "+ RemoveAlgleBracket(line_tmp.substring(0, 2))+"\n";					//document type
			String country = line_tmp.substring(2, 5);							//country
			result +="Country: "+RemoveAlgleBracket(country)+"\n";
			//result +="Given Names: "+RemoveAlgleBracket(lines_str.substring(5, 36))+"\n";							// given name
			String name_line=line_tmp.substring(5, 36);
			String[] sept_names=name_line.split("<<");
			if(sept_names.length>=2){
				result +="Last Name: "+RemoveAlgleBracket(sept_names[0])+"\n";			// last name
				result +="Given name:"+ RemoveAlgleBracket(name_line.substring(sept_names[0].length(), 31))+"\n";	// given name
			}else{
				result +="Last Name: \n";
				result +="Given name: "+RemoveAlgleBracket(line_tmp.substring(5, 36));			// given name

			}
			line_tmp = separated_lines[1];											//get second line
			result +="Document Num: "+RemoveAlgleBracket(line_tmp.substring(0, 9))+"\n";									//document number
			result +="Hash1: "+line_tmp.substring(9, 10)+"\n";					//hash 1

			result +="Nationality: "+RemoveAlgleBracket(lines_str.substring(10, 13))+"\n";							//nationality
			result +="DateOfBirth: "+line_tmp.substring(13,15)+"/"+line_tmp.substring(15,17)
					+"/"+line_tmp.substring(17,19)+"\n";
			result +="Hash2: "+line_tmp.substring(19, 20)+"\n";								//hash 2
			result +="Gender: "+line_tmp.substring(20, 21)+"\n";								//gender

			result +="ExpiryDate: "+line_tmp.substring(21, 23)+"/"+line_tmp.substring(23,25)
					+"/"+line_tmp.substring(25, 27)+"\n";								//expiry date


			result +="Hash3: "+line_tmp.substring(27, 28)+"\n";								//hash 3
			result +="OptionalData: "+RemoveAlgleBracket(lines_str.substring(28, 36))+"\n";						//OptionalData
		}

	}
	public void SetResult(int[] intData)
	{
		int i,k=0,len;
		char[] tmp = new char[100];
		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; lines = convchar2string(tmp);
		result="";
		int res=getdocType(lines);
		if(res==-1){
			result="error";

		}
		else{
			analysisMRZ(lines, res);
		}


//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; passportType = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; country = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; surname = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; givenname = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; passportnumber = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; passportchecksum = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; nationality = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; birth = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; birthchecksum = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; sex = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; expirationdate = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; expirationchecksum = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; personalnumber = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; personalnumberchecksum = convchar2string(tmp);
//		len = intData[k++]; for(i=0;i<len;++i) tmp[i] = (char)intData[k++]; tmp[i] = 0; secondrowchecksum = convchar2string(tmp);

	}
	public String  GetResultString()
	{
		String str;
//		str = lines + "\n"
//			 + passportType + "\n"
//			 + country + "\n"
//			 + surname + "\n"
//			 + givenname + "\n"
//			 + passportnumber + "\n"
//			 + passportchecksum + "\n"
//			 + nationality + "\n"
//			 + birth + "\n"
//			 + birthchecksum + "\n"
//			 + sex + "\n"
//			 + expirationdate + "\n"
//			 + expirationchecksum + "\n"
//			 + personalnumber + "\n"
//			 + personalnumberchecksum + "\n"
//			 + secondrowchecksum + "\n";
		str=result;
		return str;
	}
	public static int getByteLength(char[] str, int maxLen) {
		int i, len = 0;
		for (i = 0; i < maxLen; ++i)
		{
			if (str[i] == 0)
			{
				break;
			}
		}
		len = i;
		return len;
	}

	public static String convchar2string(char[] chstr)
	{
		int len = getByteLength(chstr, 2000);
		String outStr = new String(chstr, 0, len);// ,"UTF-8");
		return outStr;
	}
}
