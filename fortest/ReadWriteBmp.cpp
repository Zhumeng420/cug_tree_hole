#define MAXWRITE 32768

//写DIB位图到指定的BMP文件中.
BOOL WriteBMPFile(LPCTSTR FileName,BITMAPINFO *pbi,VOID *pPixel)
{
	LPBITMAPINFOHEADER pbih;
	LPBYTE lpBits;
	BYTE  *hp;
	DWORD  dwTmp,dwTotal,cb;
	BITMAPFILEHEADER hdr;    
	HANDLE hFile=INVALID_HANDLE_VALUE;
	BOOL   bResult=FALSE;
 
    pbih=(LPBITMAPINFOHEADER)pbi; 
	lpBits=(LPBYTE)pPixel;
	
    hFile=CreateFile(FileName, //创建或打开指定的BMP文件.
                     GENERIC_READ|GENERIC_WRITE, 
                     (DWORD)0, 
                     (LPSECURITY_ATTRIBUTES)NULL, 
                     CREATE_ALWAYS, 
                     FILE_ATTRIBUTE_NORMAL, 
                     (HANDLE)NULL); 
    if(hFile==INVALID_HANDLE_VALUE)
	{
		AfxMessageBox("创建BMP文件错!");
		goto WriteBMPEXT;
	}

    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) +  //计算整个文件长度. 
                 pbih->biSize + pbih->biClrUsed 
                 * sizeof(RGBQUAD) + pbih->biSizeImage); 
	hdr.bfReserved1 = 0; 
    hdr.bfReserved2 = 0;  		
	hdr.bfType = 0x4d42;  /* 0x42 = "B" 0x4d = "M" */ 
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + //计算颜色索引偏移.
                    pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD); 
 
    if (!WriteFile(hFile, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
       (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL)) //写 BITMAPFILEHEADER 到文件中.
	{
		AfxMessageBox("写BMP文件错!");
		CloseHandle(hFile);
		goto WriteBMPEXT;
	}
    if (!WriteFile(hFile, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
                  + pbih->biClrUsed * sizeof (RGBQUAD), 
                  (LPDWORD) &dwTmp, (LPOVERLAPPED) NULL)) 
	{				//写 BITMAPINFOHEADER和RGBQUAD 到文件中.
		AfxMessageBox("写BMP文件错!");
		CloseHandle(hFile);
		goto WriteBMPEXT;
	}	
    dwTotal = cb = pbih->biSizeImage; //写颜色数据到 .BMP 文件中.
    hp=lpBits; 
    while (cb > MAXWRITE) 
	{
		WriteFile(hFile,(LPSTR)hp,(int)MAXWRITE,(LPDWORD)&dwTmp,(LPOVERLAPPED) NULL);
        cb-= MAXWRITE;   hp += MAXWRITE; 
	}	
	if(cb>0) WriteFile(hFile, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, (LPOVERLAPPED) NULL);
    
	CloseHandle(hFile);
	bResult=TRUE;

WriteBMPEXT:
	return bResult;
}


BOOL ReadBMPInfo(LPCTSTR FileName,LPBITMAPINFO *BmpInfo,LPBYTE *PixelInfo)
{
	HANDLE hFile;
	DWORD  Move,Num,nNumSize;
	WORD   cClrBits;
	LPBITMAPINFO pbmi=NULL;
	LPBYTE lpBits=NULL;
    BOOL   bResult=FALSE;
	
	hFile=CreateFile(FileName, 
                     GENERIC_READ,
                     0, 
                     NULL, 
                     OPEN_EXISTING, 
                     FILE_ATTRIBUTE_NORMAL, 
                     NULL); 
 
    if(hFile==INVALID_HANDLE_VALUE) return FALSE;

	//BMP文件头.
	BITMAPFILEHEADER tmpfilehdr;
	ReadFile(hFile,&tmpfilehdr,sizeof(BITMAPFILEHEADER),&Num,NULL);
	char *type=(char *)&tmpfilehdr.bfType;
	if(*type!='B'||*++type!='M') goto END;
	
    //BMP信息头.
	BITMAPINFOHEADER tmpinfohdr;
	ReadFile(hFile,&tmpinfohdr,sizeof(BITMAPINFOHEADER),&Num,NULL);
	
	cClrBits=tmpinfohdr.biPlanes*tmpinfohdr.biBitCount;
	if(cClrBits<16) goto END;

	//填充一些缺省域的值.
	if(tmpinfohdr.biSizeImage==0)
		tmpinfohdr.biSizeImage=((tmpinfohdr.biWidth*tmpinfohdr.biBitCount)+31)/32*4*tmpinfohdr.biHeight;
	nNumSize=tmpinfohdr.biSizeImage;
		
	if(tmpinfohdr.biClrUsed==0)
	{
		if(cClrBits<24) tmpinfohdr.biClrUsed=2^cClrBits;
	}

	//申请并填充BMP头信息空间.
	if(cClrBits<24)
		pbmi=(LPBITMAPINFO)LocalAlloc(LPTR,sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*(2^cClrBits));
    else
	    pbmi=(LPBITMAPINFO)LocalAlloc(LPTR,sizeof(BITMAPINFOHEADER));
	if(!pbmi) goto END;
	
	pbmi->bmiHeader=tmpinfohdr;
	if(cClrBits<24)
	{
		RGBQUAD *pRgb=(RGBQUAD *)((LPSTR)pbmi+tmpinfohdr.biSize);
	    Move=sizeof(RGBQUAD)*(2^cClrBits);
	    ReadFile(hFile,pRgb,Move,&Num,NULL);
	}
		
	//申请并填充象素信息空间.
	lpBits=(LPBYTE)GlobalAlloc(GMEM_FIXED,nNumSize);    
	if(!lpBits) goto END;

	ReadFile(hFile,lpBits,nNumSize,&Num,NULL);
	bResult=TRUE;

END:
	CloseHandle(hFile);
	if(bResult)
	{
		*BmpInfo=pbmi;
		*PixelInfo=lpBits;
	}
	else
	{
		if(pbmi) LocalFree((HLOCAL)pbmi);
		pbmi=NULL;

		if(lpBits) GlobalFree((HGLOBAL)lpBits);
		lpBits=NULL;
	}
	return bResult;
}

/*
调用示例：

LPBITMAPINFO BmpInfo=NULL;
LPBYTE       PixelInfo=NULL;

if(!ReadBMPInfo("C:\\Road.bmp",&BmpInfo,&PixelInfo)) return FALSE;

...

if(BmpInfo!=NULL) LocalFree((HLOCAL)BmpInfo);
if(PixelInfo!=NULL) GlobalFree((HGLOBAL)PixelInfo);
*/