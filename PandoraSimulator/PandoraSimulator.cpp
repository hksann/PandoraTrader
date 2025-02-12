// cwPegasusSimulator.cpp : Defines the entry point for the Pandora Simulator console application.
//
// For more information, please visit https://github.com/pegasusTrader/PandoraTrader
//
// 除对外销售，免费提供，欢迎合规使用
//
// Please use the platform with legal and regulatory permission.
// This software is released into the public domain. You are free to use it in any way you like, except that you may not sell this source code.
// This software is provided "as is" with no expressed or implied warranty. I accept no liability for any damage or loss of business that this software may cause.
//

#include <string>
#include <cstdint> // or <stdint.h>
#include <cmath>   // for pow, sqrt
#include <unistd.h>  // for readlink
#include <cstring>   // for memset

#include "cwPegasusSimulator.h"

#ifdef REALTIME_QUOTE
#include "cwFtdMdSpi.h"
#else
#include "cwSimMdSpi.h"
#endif // REALTIME_QUOTE

#include "cwSimTradeSpi.h"
#include "cwStrategyDemo.h"
#include "cwBasicCout.h"
#include "cwVersion.h"

cwBasicCout             m_cwShow;

cwFtdcBrokerIDType      m_szMdBrokerID;
cwFtdcUserIDType        m_szMdUserID;
cwFtdcPasswordType      m_szMdPassWord;

cwPegasusSimulator      m_PegasusSimulator;

#ifdef REALTIME_QUOTE
cwFtdMdSpi              m_mdCollector;
#else
cwSimMdSpi              m_mdCollector;
#endif // REALTIME_QUOTE

cwSimTradeSpi           m_TradeChannel;
cwStrategyDemo          m_Strategy;

std::vector<std::string> m_SubscribeInstrument;

std::string             m_strStrategyConfigFile;

unsigned int PriceServerThread()
{
    m_TradeChannel.Connect(&m_PegasusSimulator);

#ifdef REALTIME_QUOTE
    m_mdCollector.m_bNoUseBasicMdUpdate = true;
#endif

    m_mdCollector.SetUserLoginField(m_szMdBrokerID, m_szMdUserID, m_szMdPassWord);
    m_mdCollector.SubscribeMarketData(m_SubscribeInstrument);

#ifdef REALTIME_QUOTE
    m_mdCollector.Connect("tcp://180.168.146.187:10131");
    sleep(2);
#else
    m_mdCollector.Connect(&m_PegasusSimulator);
#endif // REALTIME_QUOTE

    m_PegasusSimulator.SimulationStart();

    m_mdCollector.WaitForFinish();

    return 0;
}

int main()
{
    char exeFullPath[PATH_MAX];
    memset(exeFullPath, 0, PATH_MAX);
    std::string strFullPath;

    ssize_t cnt = readlink("/proc/self/exe", exeFullPath, PATH_MAX);
    if (cnt < 0 || cnt >= PATH_MAX)
    {
        printf("***Error***\n");
        exit(-1);
    }

    strFullPath = exeFullPath;
    strFullPath = strFullPath.substr(0, strFullPath.find_last_of('/'));

    strFullPath.append("/PandoraSimulatorConfig.xml");

    m_Strategy.InitialStrategy(nullptr);

    m_PegasusSimulator.InitialSimulator(strFullPath.c_str());

#ifdef REALTIME_QUOTE
    m_PegasusSimulator.SetMdSpi((void*)dynamic_cast<cwBasicMdSpi*>(&m_mdCollector));
#else
    m_PegasusSimulator.SetMdSpi((void*)(&m_mdCollector));
#endif // REALTIME_QUOTE

    m_PegasusSimulator.SetTradeSpi((void*)&m_TradeChannel);

    m_TradeChannel.RegisterBasicStrategy(dynamic_cast<cwBasicStrategy*>(&m_Strategy));

    m_mdCollector.RegisterTradeSPI(dynamic_cast<cwBasicTradeSpi*>(&m_TradeChannel));
    m_mdCollector.RegisterStrategy(dynamic_cast<cwBasicStrategy*>(&m_Strategy));

    std::thread m_PriceServerThread = std::thread(PriceServerThread);

    while (true)
    {
        sleep(3);
        m_cwShow.AddLog("%s %s 权益:%.3f 平仓盈亏:%.3f 持仓盈亏:%.3f",
            m_PegasusSimulator.m_CurrentTradingDay,
            m_PegasusSimulator.m_CurrentSimulationTime,
            m_PegasusSimulator.m_cwSettlement.m_dBalance,
            m_PegasusSimulator.m_cwSettlement.m_dCloseProfit,
            m_PegasusSimulator.m_cwSettlement.m_dPositionProfit);
    }
    return 0;
}
