// PandoraDemoStrategyTrader.cpp : Defines the entry point for the Pandora Trader console application.
//
// For more information, please visit https://github.com/pegasusTrader/PandoraTrader
//
// 本程序免费提供，欢迎合作使用
//
// Please use the platform with legal and regulatory permission.
// This software is released into the public domain. You are free to use it in any way you like, 
// except that you may not sell this source code.
// This software is provided "as is" with no expressed or implied warranty.
// I accept no liability for any damage or loss of business that this software may cause.
//

//#define EMPTYSTRATEGY  // 若需要切换到空策略，打开此宏

#include <thread>
#include <iostream>
#include <cstdio>       // for printf
#include <cstdlib>      // for exit
#include <cstring>      // for memset
#include <string>       // for std::string
#include <vector>       // for std::vector
#include <unistd.h>     // for readlink on Linux

#include "cwFtdMdSpi.h"
#include "cwFtdTradeSpi.h"

#ifdef EMPTYSTRATEGY
  #include "cwEmptyStrategy.h"
#else
  #include "cwStrategyDemo.h"
#endif

#include "tinyxml.h"
#include "cwBasicCout.h"
#include "cwVersion.h"

// ------ Windows 专用库的自动链接（VS下才有用） ------
#ifdef _MSC_VER
//#pragma comment(lib, "cwPandoraDLL.lib")
//#pragma comment(lib, "tinyxml.lib")
//#pragma comment(lib, "PandoraStrategy.lib")
#endif

// ------ Windows 专用互斥对象，仅在 Win 下有效 ------
#ifdef WIN32
#include <windows.h>
HANDLE  m_hAppMutex(NULL);
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// 全局对象（示例保留原结构）
cwFtdMdSpi     m_mdCollector;       // 行情对象
cwFtdTradeSpi  m_TradeChannel;      // 交易对象

#ifdef EMPTYSTRATEGY
  cwEmptyStrategy   m_cwStategy;    // 空策略
#else
  cwStrategyDemo    m_cwStategy;    // Demo策略
#endif

cwBasicCout     m_cwShow;           // 日志输出

// XML Config Parameter
char            m_szMdFront[64];
cwFtdcBrokerIDType   m_szMdBrokerID;
cwFtdcUserIDType     m_szMdUserID;
cwFtdcPasswordType   m_szMdPassWord;

char            m_szTdFront[64];
cwFtdcBrokerIDType   m_szTdBrokerID;
cwFtdcUserIDType     m_szTdUserID;
cwFtdcPasswordType   m_szTdPassWord;
cwFtdcProductInfoType m_szTdProductInfo;
cwFtdcAppIDType      m_szTdAppID;
cwFtdcPasswordType   m_szTdAuthCode;
char            m_szTdDllPath[MAX_PATH];

// 订阅合约列表
std::vector<std::string> m_SubscribeInstrument;

// 策略配置文件和历史数据目录
std::string     m_strStrategyConfigFile;
std::string     m_strHisDataFolder;

// --------------------- 辅助宏函数，用于读取 XML 的 attribute ---------------------
#ifdef WIN32
 #define GetCharElement(Type, Name) \
   const char * psz##Name = Element->Attribute(#Name); \
   if (psz##Name != NULL) { \
       strcpy_s(m_sz##Type##Name, psz##Name); \
   }
#else
 #define GetCharElement(Type, Name) \
   const char * psz##Name = Element->Attribute(#Name); \
   if (psz##Name != NULL) { \
       strcpy(m_sz##Type##Name, psz##Name); \
   }
#endif

// --------------------- 读取配置文件 ---------------------
bool ReadXmlConfigFile()
{
    char exeFullPath[MAX_PATH];
    memset(exeFullPath, 0, MAX_PATH);
    std::string strFullPath;

#ifdef WIN32
    // Windows 获取可执行文件路径
    WCHAR TexeFullPath[MAX_PATH] = {0};
    GetModuleFileName(NULL, TexeFullPath, MAX_PATH);
    int iLength = WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, exeFullPath, iLength, NULL, NULL);

#else
    // Linux 下使用 readlink("/proc/self/exe", ...)
    size_t cnt = readlink("/proc/self/exe", exeFullPath, MAX_PATH);
    if (cnt < 0 || cnt >= MAX_PATH)
    {
        printf("***Error***\n");
        exit(-1);
    }
#endif

    // 获取当前可执行文件所在目录
    strFullPath = exeFullPath;
    strFullPath = strFullPath.substr(0, strFullPath.find_last_of("/\\"));

#ifdef WIN32
    strFullPath.append("\\PandoraTraderConfig.xml");
#else
    strFullPath.append("/PandoraTraderConfig.xml");
#endif

    m_cwShow.AddLog("Get Account Config File : %s", strFullPath.c_str());

    TiXmlDocument doc(strFullPath.c_str());
    bool loadOkay = doc.LoadFile(TIXML_ENCODING_LEGACY);

    if (!loadOkay)
    {
        m_cwShow.AddLog("Load PandoraTraderConfig File Failed ! ");
        return false;
    }

    TiXmlNode* RootNode = doc.RootElement();
    if (RootNode)
    {
        // 读取 <User> 节点
        TiXmlNode* ChildNode = RootNode->FirstChild("User");
        if (ChildNode)
        {
            // MarketDataServer
            TiXmlNode* SubChildNode = ChildNode->FirstChild("MarketDataServer");
            if (SubChildNode)
            {
                TiXmlElement * Element = SubChildNode->ToElement();
                GetCharElement(Md, Front);
                GetCharElement(Md, BrokerID);
                GetCharElement(Md, UserID);
                GetCharElement(Md, PassWord);
            }
            // TradeServer
            SubChildNode = ChildNode->FirstChild("TradeServer");
            if (SubChildNode)
            {
                TiXmlElement * Element = SubChildNode->ToElement();
                GetCharElement(Td, Front);
                GetCharElement(Td, BrokerID);
                GetCharElement(Td, UserID);
                GetCharElement(Td, PassWord);
                GetCharElement(Td, ProductInfo);
                GetCharElement(Td, AppID);
                GetCharElement(Td, AuthCode);
                GetCharElement(Td, DllPath);
            }
        }

        // Subscription
        ChildNode = RootNode->FirstChild("Subscription");
        if (ChildNode)
        {
            TiXmlNode* SubChildNode = ChildNode->FirstChild("Instrument");
            while (SubChildNode)
            {
                TiXmlElement * Element = SubChildNode->ToElement();
                const char * pszTemp = Element->Attribute("ID");
                if (pszTemp)
                {
                    m_SubscribeInstrument.push_back(pszTemp);
                }
                SubChildNode = SubChildNode->NextSibling("Instrument");
            }
        }

        // StrategyConfigFile
        m_strStrategyConfigFile.clear();
        ChildNode = RootNode->FirstChild("StrategyConfigFile");
        if (ChildNode)
        {
            TiXmlElement * Element = ChildNode->ToElement();
            const char * pszTemp = Element->GetText();
            if (pszTemp)
            {
                m_strStrategyConfigFile = pszTemp;
            }
        }

        // HisDataFolder
        m_strHisDataFolder.clear();
        ChildNode = RootNode->FirstChild("HisDataFolder");
        if (ChildNode)
        {
            TiXmlElement * Element = ChildNode->ToElement();
            const char * pszTemp = Element->GetText();
            if (pszTemp)
            {
                m_strHisDataFolder = pszTemp;
            }
        }
    }

    return true;
}

// --------------------- 重置登录参数 ---------------------
void ResetParameter()
{
    memset(m_szMdFront,     0, sizeof(m_szMdFront));
    memset(m_szMdBrokerID,  0, sizeof(m_szMdBrokerID));
    memset(m_szMdUserID,    0, sizeof(m_szMdUserID));
    memset(m_szMdPassWord,  0, sizeof(m_szMdPassWord));

    memset(m_szTdFront,     0, sizeof(m_szTdFront));
    memset(m_szTdBrokerID,  0, sizeof(m_szTdBrokerID));
    memset(m_szTdUserID,    0, sizeof(m_szTdUserID));
    memset(m_szTdPassWord,  0, sizeof(m_szTdPassWord));
    memset(m_szTdProductInfo, 0, sizeof(m_szTdProductInfo));
    memset(m_szTdAppID,     0, sizeof(m_szTdAppID));
    memset(m_szTdAuthCode,  0, sizeof(m_szTdAuthCode));
}

// --------------------- 行情线程 ---------------------
unsigned int PriceServerThread()
{
    m_mdCollector.SetUserLoginField(m_szMdBrokerID, m_szMdUserID, m_szMdPassWord);
    m_mdCollector.SubscribeMarketData(m_SubscribeInstrument);

    m_mdCollector.Connect(m_szMdFront);
    m_mdCollector.WaitForFinish();
    return 0;
}

// --------------------- 交易线程 ---------------------
unsigned int TradeServerThread()
{
    m_TradeChannel.SetDisConnectExit(false);
    m_TradeChannel.SetSaveInstrumentDataToFile(true);
    m_TradeChannel.SetUserLoginField(m_szTdBrokerID, m_szTdUserID, m_szTdPassWord, m_szTdProductInfo);
    m_TradeChannel.SetAuthenticateInfo(m_szTdAppID, m_szTdAuthCode);

    m_TradeChannel.Connect(m_szTdFront);
    m_TradeChannel.WaitForFinish();
    return 0;
}

#ifdef WIN32
// --------------------- Windows下捕捉控制台事件的回调 ---------------------
bool CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
        printf("Ctrl-C event\n\n");
        return TRUE;

    case CTRL_CLOSE_EVENT:
        printf("Ctrl-Close event\n\n");
        m_mdCollector.DisConnect();
        m_TradeChannel.DisConnect();
        if (m_hAppMutex != NULL)
        {
            ReleaseMutex(m_hAppMutex);
            CloseHandle(m_hAppMutex);
            m_hAppMutex = NULL;
        }
        return TRUE;

    default:
        return FALSE;
    }
}
#endif // WIN32

// --------------------- 主函数入口 ---------------------
int main()
{
#ifdef WIN32
    // Windows下添加Ctrl+C等信号处理
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
    {
        printf("\nThe Control Handler is uninstalled.\n");
        return 0;
    }
#endif

    std::string strStrategyName = m_cwStategy.GetStrategyName();

    m_cwShow.AddLog("Welcome To Pandora Trader !!");
    m_cwShow.AddLog("Powered By PandoraTrader:");
    m_cwShow.AddLog("GitHub: https://github.com/pegasusTrader/PandoraTrader");
    m_cwShow.AddLog("Gitee: https://gitee.com/wuchangsheng/PandoraTrader");

    m_cwShow.AddLog("Current Version:%s", GetPandoraTraderVersion());
    m_cwShow.AddLog("Init Config From File!");

    if (!ReadXmlConfigFile())
    {
        m_cwShow.AddLog("Init Config Failed!!");
        m_cwShow.AddLog("The Program will shut down in 5s..");

        int nCnt = 0;
        while (nCnt < 5)
        {
            cwSleep(1000); // 假设cwSleep在Linux下已做跨平台适配
            m_cwShow.AddLog("%d . ", nCnt);
            nCnt++;
        }
        return -1;
    }

    m_cwShow.AddLog("User: %s ProductInfo:%s", m_szTdUserID, m_szTdProductInfo);

    // Windows下的互斥对象防止重复启动
#ifdef WIN32
    std::string strAppMutexName;
    strAppMutexName = m_szTdUserID;
    strAppMutexName.append("_");
    strAppMutexName += m_cwStategy.GetStrategyName().c_str();

    int unicodeLen = ::MultiByteToWideChar(CP_ACP, 0, strAppMutexName.c_str(), -1, NULL, 0, NULL, NULL);
    wchar_t *TAppMutexName = new wchar_t[unicodeLen + 1];
    memset(TAppMutexName, 0, (unicodeLen + 1)*sizeof(wchar_t));
    ::MultiByteToWideChar(CP_ACP, 0, strAppMutexName.c_str(), -1, (LPWSTR)TAppMutexName, unicodeLen);

    m_hAppMutex = ::CreateMutex(NULL, TRUE, TAppMutexName);
    if (m_hAppMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        m_cwShow.AddLog("检测到另一实例已在运行，请勿重复启动交易程序！");
        m_cwShow.AddLog("5秒后自动退出。");
        CloseHandle(m_hAppMutex);
        m_hAppMutex = NULL;
        delete [] TAppMutexName;

        int nCnt = 0;
        while (nCnt < 5)
        {
            cwSleep(1000);
            m_cwShow.AddLog("%d . ", nCnt);
            nCnt++;
        }
        return -1;
    }
    delete [] TAppMutexName;
#endif

    // 如果有回测历史数据目录
    if (!m_strHisDataFolder.empty())
    {
        m_cwStategy.InitialHisKindleFromHisKindleFolder(m_strHisDataFolder.c_str());
    }

    // 如果有策略配置文件，则初始化策略
    if (m_strStrategyConfigFile.empty())
        m_cwStategy.InitialStrategy(NULL);
    else
        m_cwStategy.InitialStrategy(m_strStrategyConfigFile.c_str());

    // 注册策略到交易、行情
    m_TradeChannel.RegisterBasicStrategy(dynamic_cast<cwBasicStrategy*>(&m_cwStategy));
    m_mdCollector.RegisterTradeSPI(dynamic_cast<cwBasicTradeSpi*>(&m_TradeChannel));
    m_mdCollector.RegisterStrategy(dynamic_cast<cwBasicStrategy*>(&m_cwStategy));

    // 启动行情与交易线程
    std::thread m_PriceServerThread = std::thread(PriceServerThread);
    std::thread m_TradeServerThread = std::thread(TradeServerThread);

    // 主循环，显示账户信息
    int iCnt = 0;
    while (true)
    {
        iCnt++;
        if (iCnt % 20 == 0)
        {
            // 显示当前状态
            if (iCnt % 80 == 0)
            {
                m_cwShow.AddLog("%s %s Md:%s Trade:%s",
                    m_szTdUserID, strStrategyName.c_str(),
                    m_mdCollector.GetCurrentStatusString(),
                    m_TradeChannel.GetCurrentStatusString());
            }
            cwAccountPtr pAccount = m_TradeChannel.GetAccount();
            if (pAccount)
            {
                m_cwShow.AddLog("%s Total:%.2f Available:%.2f PL:%.2f Fee:%.2f",
                    m_cwStategy.m_strCurrentUpdateTime.c_str(),
                    pAccount->Balance, pAccount->Available,
                    pAccount->CloseProfit + pAccount->PositionProfit - pAccount->Commission,
                    pAccount->Commission);
            }
        }
        cwSleep(1000); // 跨平台睡眠
    }

    return 0;
}
