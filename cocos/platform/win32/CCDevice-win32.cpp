/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32

#include "platform/CCDevice.h"
#include "platform/CCFileUtils.h"
#include "platform/CCStdC.h"

NS_CC_BEGIN

int Device::getDPI()
{
    static int dpi = -1;
    if (dpi == -1)
    {
        HDC hScreenDC = GetDC( nullptr );
        int PixelsX = GetDeviceCaps( hScreenDC, HORZRES );
        int MMX = GetDeviceCaps( hScreenDC, HORZSIZE );
        ReleaseDC( nullptr, hScreenDC );
        dpi = 254.0f*PixelsX/MMX/10;
    }
    return dpi;
}

void Device::setAccelerometerEnabled(bool isEnabled)
{}

void Device::setAccelerometerInterval(float interval)
{}

class BitmapDC
{
public:
    BitmapDC(HWND hWnd = nullptr)
        : _DC(nullptr)
        , _bmp(nullptr)
        , _font((HFONT)GetStockObject(DEFAULT_GUI_FONT))
        , _wnd(nullptr)
    {
        _wnd = hWnd;
        HDC hdc = GetDC(hWnd);
        _DC   = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
    }

    ~BitmapDC()
    {
        prepareBitmap(0, 0);
        if (_DC)
        {
            DeleteDC(_DC);
        }
        removeCustomFont();
    }

    wchar_t * utf8ToUtf16(const std::string& str)
    {
        wchar_t * pwszBuffer = nullptr;
        do
        {
            if (str.empty())
            {
                break;
            }
            // utf-8 to utf-16
            int nLen = str.size();
            int nBufLen  = nLen + 1;
            pwszBuffer = new wchar_t[nBufLen];
            CC_BREAK_IF(! pwszBuffer);
            memset(pwszBuffer,0,nBufLen);
            nLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), nLen, pwszBuffer, nBufLen);
            pwszBuffer[nLen] = '\0';
        } while (0);
        return pwszBuffer;

    }

    bool setFont(const char * pFontName = nullptr, int nSize = 0, bool enableBold = false)
    {
        bool bRet = false;
        do
        {
            std::string fontName = pFontName;
            std::string fontPath;
            HFONT       hDefFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            LOGFONTA    tNewFont = {0};
            LOGFONTA    tOldFont = {0};
            GetObjectA(hDefFont, sizeof(tNewFont), &tNewFont);
            if (!fontName.empty())
            {
                // create font from ttf file
                if (FileUtils::getInstance()->getFileExtension(fontName) == ".ttf")
                {
                    fontPath = FileUtils::getInstance()->fullPathForFilename(fontName.c_str());
                    int nFindPos = fontName.rfind("/");
                    fontName = &fontName[nFindPos+1];
                    nFindPos = fontName.rfind(".");
                    fontName = fontName.substr(0,nFindPos);
                }
                else
                {
                    auto nFindPos = fontName.rfind("/");
                    if (nFindPos != fontName.npos)
                    {
                        if (fontName.length() == nFindPos + 1)
                        {
                            fontName = "";
                        }
                        else
                        {
                            fontName = &fontName[nFindPos+1];
                        }
                    }
                }
                tNewFont.lfCharSet = DEFAULT_CHARSET;
                strcpy_s(tNewFont.lfFaceName, LF_FACESIZE, fontName.c_str());
            }

            if (nSize)
            {
                tNewFont.lfHeight = -nSize;
            }

            if (enableBold)
            {
                tNewFont.lfWeight = FW_BOLD;
            }
            else
            {
                tNewFont.lfWeight = FW_NORMAL;
            }

            GetObjectA(_font, sizeof(tOldFont), &tOldFont);

            if (tOldFont.lfHeight == tNewFont.lfHeight
                && tOldFont.lfWeight == tNewFont.lfWeight
                && 0 == strcmp(tOldFont.lfFaceName, tNewFont.lfFaceName))
            {
                bRet = true;
                break;
            }

            // delete old font
            removeCustomFont();

            if (fontPath.size() > 0)
            {
                _curFontPath = fontPath;
                wchar_t * pwszBuffer = utf8ToUtf16(_curFontPath);
                if (pwszBuffer)
                {
                    if(AddFontResource(pwszBuffer))
                    {
                        SendMessage( _wnd, WM_FONTCHANGE, 0, 0);
                    }
                    delete [] pwszBuffer;
                    pwszBuffer = nullptr;
                }
            }

            _font = nullptr;

            // disable Cleartype
            tNewFont.lfQuality = ANTIALIASED_QUALITY;
		
            // create new font
            _font = CreateFontIndirectA(&tNewFont);
            if (! _font)
            {
                // create failed, use default font
                _font = hDefFont;
                break;
            }

            bRet = true;
        } while (0);
        return bRet;
    }

    SIZE sizeWithText(const wchar_t * pszText, 
        int nLen, 
        DWORD dwFmt,
        const char* fontName,
        int textSize,
        LONG nWidthLimit,
        LONG nHeightLimit,
        bool enableWrap,
        int overflow)
    {
        SIZE tRet = {0};
        do
        {
            CC_BREAK_IF(! pszText || nLen <= 0);

            RECT rc = {0, 0, 0, 0};
            DWORD dwCalcFmt = DT_CALCRECT;
            if (!enableWrap)
            {
                dwCalcFmt |= DT_SINGLELINE;
            }

            if (nWidthLimit > 0)
            {
                rc.right = nWidthLimit;
                dwCalcFmt |= DT_WORDBREAK | DT_EDITCONTROL
                    | (dwFmt & DT_CENTER)
                    | (dwFmt & DT_RIGHT);
            }
            if (overflow == 2) 
            {
                LONG actualWidth = nWidthLimit + 1;
                LONG actualHeight = nHeightLimit + 1;
                int newFontSize = textSize + 1;

                while (actualWidth > nWidthLimit || actualHeight > nHeightLimit)
                {
                    if (newFontSize <= 0)
                    {
                        break;
                    }
                    this->setFont(fontName, newFontSize);
                    // use current font to measure text extent
                    HGDIOBJ hOld = SelectObject(_DC, _font);
                    rc.right = nWidthLimit;
                    // measure text size
                    DrawTextW(_DC, pszText, nLen, &rc, dwCalcFmt);
                    SelectObject(_DC, hOld);

                    actualWidth = rc.right;
                    actualHeight = rc.bottom;
                    newFontSize = newFontSize - 1;
                }
            }
            else
            {
                // use current font to measure text extent
                HGDIOBJ hOld = SelectObject(_DC, _font);

                // measure text size
                DrawTextW(_DC, pszText, nLen, &rc, dwCalcFmt);
                SelectObject(_DC, hOld);
            }

            tRet.cx = rc.right;
            tRet.cy = rc.bottom;
          
        } while (0);

        return tRet;
    }

    bool prepareBitmap(int nWidth, int nHeight)
    {
        // release bitmap
        if (_bmp)
        {
            DeleteObject(_bmp);
            _bmp = nullptr;
        }
        if (nWidth > 0 && nHeight > 0)
        {
            _bmp = CreateBitmap(nWidth, nHeight, 1, 32, nullptr);
            if (! _bmp)
            {
                return false;
            }
        }
        return true;
    }

	HDC getDC() const
	{
		return _DC;
	}

	HBITMAP getBitmap() const
	{
		return _bmp;
	}

	HDC _DC;
	HBITMAP _bmp;
private:

    friend class Image;
    HFONT   _font;
    HWND    _wnd;
    std::string _curFontPath;

    void removeCustomFont()
    {
        HFONT hDefFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (hDefFont != _font)
        {
            DeleteObject(_font);
            _font = hDefFont;
        }
        // release temp font resource
        if (_curFontPath.size() > 0)
        {
            wchar_t * pwszBuffer = utf8ToUtf16(_curFontPath);
            if (pwszBuffer)
            {
                RemoveFontResource(pwszBuffer);
                SendMessage( _wnd, WM_FONTCHANGE, 0, 0);
                delete [] pwszBuffer;
                pwszBuffer = nullptr;
            }
            _curFontPath.clear();
        }
    }
};

static BitmapDC& sharedBitmapDC()
{
    static BitmapDC s_BmpDC;
    return s_BmpDC;
}

void Device::setKeepScreenOn(bool value)
{
    CC_UNUSED_PARAM(value);
}

void Device::vibrate(float duration)
{
    CC_UNUSED_PARAM(duration);
}

float Device::getBatteryLevel()
{
    return 1.0f;
}

Device::NetworkType Device::getNetworkType()
{
    return Device::NetworkType::LAN;
}

NS_CC_END

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
