/+------------------------------------------------------------------+
//|                    HighProfitEA_v4_BTC_ULTIMATE.mq5              |
//|                  Ultimate Bitcoin Trading System                  |
//|           Professional Grade - Maximum Performance Edition        |
//+------------------------------------------------------------------+
#property copyright "Copyright 2024, Bitcoin Profit Systems"
#property link      "https://www.btcprofitsystems.com"
#property version   "4.03"
#property strict
#property description "Advanced Multi-Strategy Bitcoin EA"
#property description "Optimized for BTC volatility with institutional-grade risk management"

//+------------------------------------------------------------------+
//| WHAT'S NEW IN v4.03 - CRITICAL FIXES                             |
//| 🔧 FIXED: Volatility filter with tolerance                       |
//| 🔧 FIXED: Relaxed RSI requirements for trend signals             |
//| 🔧 FIXED: Enhanced diagnostic logging with strategy tracking     |
//| 🔧 OPTIMIZED: Default parameters for better signal generation    |
//| ⭐ NEW: Strategy performance tracking (24H summaries)            |
//+------------------------------------------------------------------+

//--- Enhanced Constants
const double TREND_STRENGTH_EXTREME = 0.90;
const double TREND_STRENGTH_VERY_HIGH = 0.80;
const double TREND_STRENGTH_HIGH = 0.70;
const double TREND_STRENGTH_MEDIUM = 0.60;
const double MOMENTUM_EXTREME = 0.85;
const double MOMENTUM_VERY_STRONG = 0.75;
const double MOMENTUM_STRONG = 0.65;
const double ATR_SL_TIGHT = 2.0;
const double ATR_SL_NORMAL = 2.5;
const double ATR_SL_WIDE = 3.0;
const double VOLATILITY_EXTREME = 2.0;
const double VOLATILITY_VERY_HIGH = 1.7;
const double VOLATILITY_HIGH = 1.4;
const int STRUCTURE_LOOKBACK = 30;
const double VOLATILITY_TOLERANCE = 0.05; // 0.05% tolerance for float comparison

//--- Risk Management Inputs
input group "═══ 💰 RISK MANAGEMENT (BTC OPTIMIZED) ═══"
input double      RiskPercent = 5;              // Risk per trade (%) [BTC: 0.3-0.7%]
input double      MaxDailyLoss = 50;             // Max daily loss (%) [BTC: 2-3%]
input double      MaxWeeklyLoss = 100.0;            // Max weekly loss (%)
input double      MaxDrawdown = 10.0;             // Max drawdown (%)
input double      MinRiskReward = 2.0;            // Minimum R:R ratio
input int         MaxPositions = 10;               // Max concurrent positions
input int         MaxDailyTrades = 9;             // Max trades per day
input int         MaxConsecutiveLosses = 3;       // Emergency stop losses
input bool        UseVolatilityScaling = true;    // Scale risk by volatility

//--- Strategy Configuration
input group "═══ 🎯 STRATEGY SETTINGS ═══"
input bool        UseTrendStrategy = true;        // Trend Following
input bool        UseBreakoutStrategy = true;     // Breakout Trading
input bool        UseMomentumStrategy = true;     // Momentum Trading
input bool        UseStructureStrategy = true;    // Market Structure
input bool        RequireMultipleSignals = false; // ⭐ FIXED: Changed to false
input bool        UseMultiTimeframe = false;      // ⭐ FIXED: Disabled for testing
input ENUM_TIMEFRAMES PrimaryTF = PERIOD_H1;      // Primary timeframe
input ENUM_TIMEFRAMES ConfirmTF = PERIOD_H4;      // Confirmation timeframe

//--- Technical Indicators
input group "═══ 📊 INDICATORS (BTC TUNED) ═══"
input int         EMA_Fast = 12;                  // Fast EMA
input int         EMA_Medium = 26;                // Medium EMA
input int         EMA_Slow = 50;                  // Slow EMA
input int         EMA_Trend = 100;                // Trend EMA
input int         EMA_LongTerm = 200;             // Long-term EMA
input int         RSI_Period = 14;                // RSI Period
input int         RSI_OB = 75;                    // ⭐ FIXED: Raised from 70
input int         RSI_OS = 25;                    // ⭐ FIXED: Lowered from 30
input int         ATR_Period = 14;                // ATR Period
input int         ADX_Period = 14;                // ADX Period
input double      MinADX_Trend = 18.0;            // ⭐ FIXED: Lowered from 20.0
input double      MinADX_Strong = 28.0;           // ⭐ FIXED: Lowered from 30.0
input int         BB_Period = 20;                 // Bollinger Bands period
input double      BB_Deviation = 2.0;             // BB std deviation

//--- Advanced Profit Management
input group "═══ 🎯 PROFIT MANAGEMENT (4-LEVEL) ═══"
input bool        UseDynamicTP = true;            // Dynamic TP calculation
input double      BaseRR = 2.0;                   // Base R:R ratio
input double      MaxRR = 6.0;                    // Maximum R:R ratio
input bool        UsePartialTP = true;            // Partial profit taking
input double      TP1_Percent = 25.0;             // Close 25% at TP1
input double      TP1_RR = 1.5;                   // TP1 at 1.5R
input double      TP2_Percent = 25.0;             // Close 25% at TP2
input double      TP2_RR = 2.5;                   // TP2 at 2.5R
input double      TP3_Percent = 25.0;             // Close 25% at TP3
input double      TP3_RR = 4.0;                   // TP3 at 4.0R
input double      TP4_Percent = 25.0;             // Close 25% at TP4
input double      TP4_RR = 6.0;                   // TP4 at 6.0R (runner)
input bool        MoveToBreakeven = true;         // Move to breakeven
input double      BreakevenTrigger = 0.20;        // BE at 20% of TP
input double      BreakevenBuffer = 10;           // BE buffer (points)
input bool        UseTrailing = true;             // Trailing stop
input double      TrailingStart_RR = 2.0;         // Start trailing at 2R
input double      TrailingDistance_ATR = 1.5;     // Trailing distance (ATR)

//--- Session & Time Management
input group "═══ ⏰ TRADING SESSIONS ═══"
input bool        TradeAsianSession = false;      // Asian (23:00-08:00 GMT)
input bool        TradeLondonSession = true;      // London (07:00-16:00 GMT)
input bool        TradeNYSession = true;          // New York (12:00-21:00 GMT)
input bool        AvoidWeekends = true;           // No Friday PM/Sunday PM
input int         FridayCloseHour = 14;           // Friday close hour
input int         SundayOpenHour = 20;            // Sunday open hour

//--- Filters & Protection
input group "═══ 🛡️ FILTERS & PROTECTION ═══"
input bool        UseSpreadFilter = true;         // Spread filter
input double      MaxSpreadPips = 0.0;            // Max spread (0=auto)
input double      MaxSpreadATR = 0.4;             // Max spread/ATR ratio
input bool        UseVolatilityFilter = true;     // Volatility filter
input double      MinVolatility = 0.3;            // ⭐ FIXED: Lowered from 0.5
input double      MaxVolatility = 3.0;            // Max ATR/price ratio
input bool        UseNewsFilter = false;          // News avoidance
input int         NewsAvoidMinutes = 30;          // Minutes before/after news
input bool        UseDrawdownProtection = true;   // Drawdown protection

//--- Money Management
input group "═══ 💵 MONEY MANAGEMENT ═══"
input double      FixedLot = 0.0;                 // Fixed lot (0=auto)
input double      MaxLotSize = 10.0;              // Maximum lot size
input int         Slippage = 100;                 // Max slippage (points)
input int         MagicNumber = 440001;           // Magic number
input string      TradeComment = "HPEA_BTC_v4";   // Trade comment

//--- Display & Notifications
input group "═══ 📱 DISPLAY & ALERTS ═══"
input bool        ShowDashboard = true;           // Show info panel
input bool        SendAlerts = true;              // Send alerts
input bool        SendPushNotifications = false;  // Push notifications
input bool        EnableDebugLogs = true;         // Print detailed diagnostics
input bool        ShowStrategyStats = true;       // ⭐ NEW: Show 24H strategy stats

//--- Global Variables
int handleEMA_Fast, handleEMA_Medium, handleEMA_Slow, handleEMA_Trend, handleEMA_LongTerm;
int handleRSI, handleATR, handleADX, handleBB;
int handleEMA_Fast_HTF, handleEMA_Slow_HTF, handleRSI_HTF;

double ema_fast[], ema_medium[], ema_slow[], ema_trend[], ema_longterm[];
double rsi[], atr[], adx[], bb_upper[], bb_middle[], bb_lower[];
double ema_fast_htf[], ema_slow_htf[], rsi_htf[];

datetime lastBarTime = 0;
datetime lastTradeTime = 0;
datetime lastReasonLog = 0;
double dailyProfit = 0;
double weeklyProfit = 0;
double startingDailyBalance = 0;
double startingWeeklyBalance = 0;
datetime lastDayCheck = 0;
datetime lastWeekCheck = 0;
bool dailyLimitReached = false;
bool weeklyLimitReached = false;
int dailyTradeCount = 0;
int consecutiveLosses = 0;
int consecutiveWins = 0;
double peakEquity = 0;
double peakBalance = 0;
bool emergencyStop = false;
datetime lastSpreadWarning = 0;
double maxDrawdownReached = 0;

// Performance tracking
int totalTrades = 0;
int winningTrades = 0;
int losingTrades = 0;
double totalProfit = 0;
double totalLoss = 0;
double largestWin = 0;
double largestLoss = 0;

// ⭐ NEW: Strategy signal tracking
int trendSignalCount = 0;
int breakoutSignalCount = 0;
int momentumSignalCount = 0;
int structureSignalCount = 0;
int barCountFor24H = 0;

//--- Position Tracking Enhanced
struct PositionInfo
{
   ulong ticket;
   bool tp1_hit;
   bool tp2_hit;
   bool tp3_hit;
   bool tp4_hit;
   bool be_moved;
   bool trailing_active;
   double original_volume;
   double current_volume;
   double entry_price;
   double highest_price;
   double lowest_price;
   datetime entry_time;
   string entry_reason;
   double entry_atr;
   double max_favorable_excursion;
   double max_adverse_excursion;
};

PositionInfo positionTracking[];

//+------------------------------------------------------------------+
//| Expert initialization                                             |
//+------------------------------------------------------------------+
int OnInit()
{
   Print("╔══════════════════════════════════════════════════════╗");
   Print("║  🚀 BITCOIN ULTIMATE TRADING SYSTEM v4.03           ║");
   Print("║      FIXED VERSION - Enhanced Signal Generation      ║");
   Print("╚══════════════════════════════════════════════════════╝");
   Print("");

   // Validate Bitcoin symbol
   string symbol = _Symbol;
   bool isBTC = (StringFind(symbol, "BTC") >= 0 || StringFind(symbol, "XBT") >= 0);

   if(isBTC)
   {
      Print("✅ Bitcoin Trading Mode Activated");
      Print("   Symbol: ", symbol);
      Print("   Broker: ", AccountInfoString(ACCOUNT_COMPANY));
   }
   else
   {
      Print("⚠️ WARNING: Non-BTC symbol detected!");
      Print("   This EA is optimized specifically for Bitcoin");
      Print("   Current symbol: ", symbol);
      Print("   Settings may need adjustment");
   }
   Print("");

   // Risk warnings
   if(RiskPercent > 1.0)
   {
      Print("╔══════════════════════════════════════════════════════╗");
      Print("║  ⚠️ HIGH RISK WARNING - BITCOIN VOLATILITY          ║");
      Print("╚══════════════════════════════════════════════════════╝");
      Print("Current Risk: ", RiskPercent, "% per trade");
      Print("BTC Recommended: 0.3-0.7%");
      Print("");
      Print("⚠️  With BTC volatility:");
      Print("   • Risk feels 3-5x higher than forex");
      Print("   • 5 losses = ", DoubleToString(5 * RiskPercent, 1), "% account drawdown");
      Print("   • Consider reducing risk for stability");
      Print("═════════════════════════════════════════════════════");
      Print("");
   }

   // Initialize primary indicators
   handleEMA_Fast = iMA(_Symbol, PrimaryTF, EMA_Fast, 0, MODE_EMA, PRICE_CLOSE);
   handleEMA_Medium = iMA(_Symbol, PrimaryTF, EMA_Medium, 0, MODE_EMA, PRICE_CLOSE);
   handleEMA_Slow = iMA(_Symbol, PrimaryTF, EMA_Slow, 0, MODE_EMA, PRICE_CLOSE);
   handleEMA_Trend = iMA(_Symbol, PrimaryTF, EMA_Trend, 0, MODE_EMA, PRICE_CLOSE);
   handleEMA_LongTerm = iMA(_Symbol, PrimaryTF, EMA_LongTerm, 0, MODE_EMA, PRICE_CLOSE);
   handleRSI = iRSI(_Symbol, PrimaryTF, RSI_Period, PRICE_CLOSE);
   handleATR = iATR(_Symbol, PrimaryTF, ATR_Period);
   handleADX = iADX(_Symbol, PrimaryTF, ADX_Period);
   handleBB = iBands(_Symbol, PrimaryTF, BB_Period, 0, BB_Deviation, PRICE_CLOSE);

   // Initialize higher timeframe indicators
   if(UseMultiTimeframe)
   {
      handleEMA_Fast_HTF = iMA(_Symbol, ConfirmTF, EMA_Fast, 0, MODE_EMA, PRICE_CLOSE);
      handleEMA_Slow_HTF = iMA(_Symbol, ConfirmTF, EMA_Slow, 0, MODE_EMA, PRICE_CLOSE);
      handleRSI_HTF = iRSI(_Symbol, ConfirmTF, RSI_Period, PRICE_CLOSE);
   }

   // Validate indicators
   if(handleEMA_Fast == INVALID_HANDLE || handleEMA_Medium == INVALID_HANDLE ||
      handleEMA_Slow == INVALID_HANDLE || handleEMA_Trend == INVALID_HANDLE ||
      handleRSI == INVALID_HANDLE || handleATR == INVALID_HANDLE ||
      handleADX == INVALID_HANDLE || handleBB == INVALID_HANDLE)
   {
      Print("❌ ERROR: Failed to initialize indicators");
      return(INIT_FAILED);
   }

   if(UseMultiTimeframe && (handleEMA_Fast_HTF == INVALID_HANDLE ||
      handleEMA_Slow_HTF == INVALID_HANDLE || handleRSI_HTF == INVALID_HANDLE))
   {
      Print("❌ ERROR: Failed to initialize HTF indicators");
      return(INIT_FAILED);
   }

   // Setup arrays
   ArraySetAsSeries(ema_fast, true);
   ArraySetAsSeries(ema_medium, true);
   ArraySetAsSeries(ema_slow, true);
   ArraySetAsSeries(ema_trend, true);
   ArraySetAsSeries(ema_longterm, true);
   ArraySetAsSeries(rsi, true);
   ArraySetAsSeries(atr, true);
   ArraySetAsSeries(adx, true);
   ArraySetAsSeries(bb_upper, true);
   ArraySetAsSeries(bb_middle, true);
   ArraySetAsSeries(bb_lower, true);

   if(UseMultiTimeframe)
   {
      ArraySetAsSeries(ema_fast_htf, true);
      ArraySetAsSeries(ema_slow_htf, true);
      ArraySetAsSeries(rsi_htf, true);
   }

   // Initialize tracking
   ArrayResize(positionTracking, 0);
   startingDailyBalance = AccountInfoDouble(ACCOUNT_BALANCE);
   startingWeeklyBalance = startingDailyBalance;
   peakEquity = AccountInfoDouble(ACCOUNT_EQUITY);
   peakBalance = startingDailyBalance;
   lastDayCheck = TimeCurrent();
   lastWeekCheck = TimeCurrent();

   // Display configuration
   Print("╔══════════════════════════════════════════════════════╗");
   Print("║            v4.03 FIXES APPLIED                       ║");
   Print("╚══════════════════════════════════════════════════════╝");
   Print("   ✅ Volatility Filter: Tolerance added (±0.05%)");
   Print("   ✅ Multiple Signals: ", RequireMultipleSignals ? "REQUIRED" : "OPTIONAL");
   Print("   ✅ Min Volatility: ", MinVolatility, "% (was 0.5%)");
   Print("   ✅ Min ADX Trend: ", MinADX_Trend, " (was 20.0)");
   Print("   ✅ RSI Range: ", RSI_OS, "-", RSI_OB, " (was 30-70)");
   Print("   ✅ HTF Confirmation: ", UseMultiTimeframe ? "ENABLED" : "DISABLED");
   Print("   ✅ Debug Logging: ", EnableDebugLogs ? "ENABLED" : "DISABLED");
   Print("   ✅ Strategy Tracking: ", ShowStrategyStats ? "ENABLED" : "DISABLED");
   Print("");
   Print("╔══════════════════════════════════════════════════════╗");
   Print("║  ✅ INITIALIZATION COMPLETE - READY TO TRADE        ║");
   Print("╚══════════════════════════════════════════════════════╝");
   Print("");

   return(INIT_SUCCEEDED);
}

//+------------------------------------------------------------------+
//| Expert deinitialization                                           |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
{
   // Release indicators
   IndicatorRelease(handleEMA_Fast);
   IndicatorRelease(handleEMA_Medium);
   IndicatorRelease(handleEMA_Slow);
   IndicatorRelease(handleEMA_Trend);
   IndicatorRelease(handleEMA_LongTerm);
   IndicatorRelease(handleRSI);
   IndicatorRelease(handleATR);
   IndicatorRelease(handleADX);
   IndicatorRelease(handleBB);

   if(UseMultiTimeframe)
   {
      IndicatorRelease(handleEMA_Fast_HTF);
      IndicatorRelease(handleEMA_Slow_HTF);
      IndicatorRelease(handleRSI_HTF);
   }

   // Delete dashboard
   if(ShowDashboard)
   {
      ObjectsDeleteAll(0, "HPEA_");
   }

   // Final report
   Print("╔══════════════════════════════════════════════════════╗");
   Print("║           EA STOPPED - FINAL REPORT                  ║");
   Print("╚══════════════════════════════════════════════════════╝");
   Print("Reason: ", GetDeinitReasonText(reason));
   Print("");
   Print("📊 PERFORMANCE:");
   Print("   Total Trades: ", totalTrades);
   Print("   Wins: ", winningTrades, " (", DoubleToString(totalTrades > 0 ? (winningTrades * 100.0 / totalTrades) : 0, 1), "%)");
   Print("   Losses: ", losingTrades);
   Print("   Consecutive Losses: ", consecutiveLosses);
   Print("   Total Profit: $", DoubleToString(totalProfit, 2));
   Print("══════════════════════════════════════════════════════");
}

//+------------------------------------------------------------------+
//| Get deinit reason                                                 |
//+------------------------------------------------------------------+
string GetDeinitReasonText(int reason)
{
   switch(reason)
   {
      case REASON_PROGRAM: return "Program terminated";
      case REASON_REMOVE: return "EA removed";
      case REASON_RECOMPILE: return "EA recompiled";
      case REASON_CHARTCHANGE: return "Chart changed";
      case REASON_CHARTCLOSE: return "Chart closed";
      case REASON_PARAMETERS: return "Parameters changed";
      case REASON_ACCOUNT: return "Account changed";
      default: return "Unknown";
   }
}

//+------------------------------------------------------------------+
//| Expert tick function                                              |
//+------------------------------------------------------------------+
void OnTick()
{
   // Emergency stop check
   if(emergencyStop)
   {
      if(ShowDashboard) UpdateDashboard();
      return;
   }

   // New bar check
   if(!IsNewBar())
   {
      if(UpdateIndicators())
      {
         ManageDynamicPositions();
         PrintNoEntryReason();
      }
      if(ShowDashboard) UpdateDashboard();
      return;
   }

   // ⭐ NEW: Strategy tracking
   if(ShowStrategyStats)
   {
      TrackStrategySignals();
   }

   // Daily/weekly reset
   CheckDailyProfit();
   CheckWeeklyProfit();

   // Update indicators first
   if(!UpdateIndicators())
   {
      if(ShowDashboard) UpdateDashboard();
      return;
   }

   // Risk checks
   if(!CheckRiskLimits()) return;

   // Trading conditions
   if(!IsTradingSession()) return;
   if(UseSpreadFilter && !IsSpreadAcceptable()) return;
   if(UseVolatilityFilter && !IsVolatilityAcceptable()) return;
   if(dailyTradeCount >= MaxDailyTrades) return;

   // Position limits
   int openPos = CountOpenPositions();
   if(openPos >= MaxPositions) return;

   // Execute trading logic
   CheckAndExecuteSignals();
   ManageDynamicPositions();
   CleanupPositionTracking();

   // Update display
   if(ShowDashboard) UpdateDashboard();
}

//+------------------------------------------------------------------+
//| ⭐ NEW: Track strategy signals (FIXED - Safe array access)        |
//+------------------------------------------------------------------+
void TrackStrategySignals()
{
   // ⭐ CRITICAL FIX: Verify all arrays are populated before checking strategies
   if(ArraySize(ema_fast) < 3 || ArraySize(ema_slow) < 3 ||
      ArraySize(rsi) < 3 || ArraySize(atr) < 3 || ArraySize(adx) < 3)
   {
      return; // Skip if indicators not ready
   }

   if(atr[0] <= 0 || atr[1] <= 0 || atr[2] <= 0) return;

   barCountFor24H++;

   if(UseTrendStrategy)
   {
      if(CheckTrendLong() || CheckTrendShort()) trendSignalCount++;
   }

   if(UseBreakoutStrategy)
   {
      if(CheckBreakoutLong() || CheckBreakoutShort()) breakoutSignalCount++;
   }

   if(UseMomentumStrategy)
   {
      if(CheckMomentumLong() || CheckMomentumShort()) momentumSignalCount++;
   }

   if(UseStructureStrategy)
   {
      if(CheckStructureLong() || CheckStructureShort()) structureSignalCount++;
   }

   // Print summary every 24 bars (24 hours on H1)
   if(barCountFor24H >= 24)
   {
      Print("╔══════════════════════════════════════════════════════╗");
      Print("║         24-HOUR STRATEGY SIGNAL SUMMARY              ║");
      Print("╚══════════════════════════════════════════════════════╝");
      Print("   Trend signals: ", trendSignalCount);
      Print("   Breakout signals: ", breakoutSignalCount);
      Print("   Momentum signals: ", momentumSignalCount);
      Print("   Structure signals: ", structureSignalCount);
      Print("   Total signals: ", trendSignalCount + breakoutSignalCount +
            momentumSignalCount + structureSignalCount);
      Print("   Trades executed: ", dailyTradeCount);
      Print("══════════════════════════════════════════════════════");

      // Reset counters
      trendSignalCount = 0;
      breakoutSignalCount = 0;
      momentumSignalCount = 0;
      structureSignalCount = 0;
      barCountFor24H = 0;
   }
}

//+------------------------------------------------------------------+
//| ⭐ FIXED: Enhanced diagnostic logging                             |
//+------------------------------------------------------------------+
void PrintNoEntryReason()
{
   if(!EnableDebugLogs) return;

   if(TimeCurrent() - lastReasonLog < 60) return;
   lastReasonLog = TimeCurrent();

   string reason = "";

   // 1. Hard Blocks
   if(emergencyStop) reason = "⛔ Emergency Stop Active";
   else if(dailyLimitReached) reason = "⛔ Daily Loss Limit Reached";
   else if(weeklyLimitReached) reason = "⛔ Weekly Loss Limit Reached";
   else if(!IsTradingSession()) reason = "⛔ Outside Trading Session";
   else if(dailyTradeCount >= MaxDailyTrades)
      reason = StringFormat("⛔ Daily Trade Cap: %d/%d", dailyTradeCount, MaxDailyTrades);
   else if(CountOpenPositions() >= MaxPositions)
      reason = "⛔ Max Positions Reached";

   // 2. Filter Checks with DETAILED VALUES
   if(reason == "" && UseSpreadFilter)
   {
      double spread = SymbolInfoInteger(_Symbol, SYMBOL_SPREAD) * _Point;
      double maxSpread = (MaxSpreadPips > 0) ? MaxSpreadPips * _Point * 10 : atr[0] * MaxSpreadATR;

      if(spread > maxSpread)
         reason = StringFormat("⚠️ Spread: %.1f > %.1f pts",
                               spread/_Point, maxSpread/_Point);
   }

   if(reason == "" && UseVolatilityFilter)
   {
      double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);
      if(currentPrice > 0 && atr[0] > 0)
      {
         double volatilityRatio = (atr[0] / currentPrice) * 100.0;

         if(volatilityRatio < (MinVolatility - VOLATILITY_TOLERANCE) ||
            volatilityRatio > (MaxVolatility + VOLATILITY_TOLERANCE))
         {
            reason = StringFormat("⚠️ Volatility: %.3f%% (Need: %.1f-%.1f%%)",
                                  volatilityRatio, MinVolatility, MaxVolatility);
         }
      }
   }

   // 3. Strategy Status with Signal Counting
   if(reason == "")
   {
      int longSignals = 0;
      int shortSignals = 0;
      string longReasons = "";
      string shortReasons = "";

      // Check each strategy
      if(UseTrendStrategy)
      {
         if(CheckTrendLong()) { longSignals++; longReasons += "TREND "; }
         if(CheckTrendShort()) { shortSignals++; shortReasons += "TREND "; }
      }

      if(UseBreakoutStrategy)
      {
         if(CheckBreakoutLong()) { longSignals++; longReasons += "BREAK "; }
         if(CheckBreakoutShort()) { shortSignals++; shortReasons += "BREAK "; }
      }

      if(UseMomentumStrategy)
      {
         if(CheckMomentumLong()) { longSignals++; longReasons += "MOM "; }
         if(CheckMomentumShort()) { shortSignals++; shortReasons += "MOM "; }
      }

      if(UseStructureStrategy)
      {
         if(CheckStructureLong()) { longSignals++; longReasons += "STRUCT "; }
         if(CheckStructureShort()) { shortSignals++; shortReasons += "STRUCT "; }
      }

      int requiredSignals = RequireMultipleSignals ? 2 : 1;

      if(longSignals >= requiredSignals)
         reason = StringFormat("🟢 LONG SIGNAL: %d strategies (%s)", longSignals, longReasons);
      else if(shortSignals >= requiredSignals)
         reason = StringFormat("🔴 SHORT SIGNAL: %d strategies (%s)", shortSignals, shortReasons);
      else if(longSignals > 0 || shortSignals > 0)
         reason = StringFormat("⚡ Partial Signal (Need %d): L=%d (%s) S=%d (%s)",
                              requiredSignals, longSignals, longReasons, shortSignals, shortReasons);
      else
      {
         string trend = (ema_fast[0] > ema_slow[0]) ? "Bullish" : "Bearish";
         double volatilityRatio = (atr[0] / SymbolInfoDouble(_Symbol, SYMBOL_BID)) * 100;
         reason = StringFormat("⏳ Scanning... Trend: %s | RSI: %.1f | ADX: %.1f | Vol: %.2f%%",
                              trend, rsi[0], adx[0], volatilityRatio);
      }
   }

   Print(reason);
}

//+------------------------------------------------------------------+
//| Check if new bar formed                                           |
//+------------------------------------------------------------------+
bool IsNewBar()
{
   datetime currentBarTime = iTime(_Symbol, PrimaryTF, 0);
   if(currentBarTime != lastBarTime)
   {
      lastBarTime = currentBarTime;
      return true;
   }
   return false;
}

//+------------------------------------------------------------------+
//| Update all indicators                                             |
//+------------------------------------------------------------------+
bool UpdateIndicators()
{
   if(Bars(_Symbol, PrimaryTF) < 210) return false;

   if(CopyBuffer(handleEMA_Fast, 0, 0, 3, ema_fast) < 3) return false;
   if(CopyBuffer(handleEMA_Medium, 0, 0, 3, ema_medium) < 3) return false;
   if(CopyBuffer(handleEMA_Slow, 0, 0, 3, ema_slow) < 3) return false;
   if(CopyBuffer(handleEMA_Trend, 0, 0, 3, ema_trend) < 3) return false;
   if(CopyBuffer(handleEMA_LongTerm, 0, 0, 3, ema_longterm) < 3) return false;
   if(CopyBuffer(handleRSI, 0, 0, 3, rsi) < 3) return false;
   if(CopyBuffer(handleATR, 0, 0, 3, atr) < 3) return false;
   if(CopyBuffer(handleADX, 0, 0, 3, adx) < 3) return false;
   if(CopyBuffer(handleBB, 0, 0, 3, bb_upper) < 3) return false;
   if(CopyBuffer(handleBB, 1, 0, 3, bb_middle) < 3) return false;
   if(CopyBuffer(handleBB, 2, 0, 3, bb_lower) < 3) return false;

   if(atr[0] <= 0 || atr[1] <= 0 || atr[2] <= 0) return false;

   if(UseMultiTimeframe)
   {
      if(Bars(_Symbol, ConfirmTF) < 210) return false;

      if(CopyBuffer(handleEMA_Fast_HTF, 0, 0, 2, ema_fast_htf) < 2) return false;
      if(CopyBuffer(handleEMA_Slow_HTF, 0, 0, 2, ema_slow_htf) < 2) return false;
      if(CopyBuffer(handleRSI_HTF, 0, 0, 2, rsi_htf) < 2) return false;
   }

   return true;
}

//+------------------------------------------------------------------+
//| Check risk limits                                                 |
//+------------------------------------------------------------------+
bool CheckRiskLimits()
{
   double maxDailyLoss = -(startingDailyBalance * MaxDailyLoss / 100);
   if(dailyProfit <= maxDailyLoss)
   {
      if(!dailyLimitReached)
      {
         Print("⛔ DAILY LOSS LIMIT REACHED!");
         Print("   Loss: $", DoubleToString(dailyProfit, 2));
         Print("   Limit: $", DoubleToString(maxDailyLoss, 2));
         dailyLimitReached = true;
         CloseAllPositions("Daily limit");
         if(SendAlerts) Alert("Daily loss limit reached!");
      }
      return false;
   }

   double maxWeeklyLoss = -(startingWeeklyBalance * MaxWeeklyLoss / 100);
   if(weeklyProfit <= maxWeeklyLoss)
   {
      if(!weeklyLimitReached)
      {
         Print("⛔ WEEKLY LOSS LIMIT REACHED!");
         Print("   Loss: $", DoubleToString(weeklyProfit, 2));
         Print("   Limit: $", DoubleToString(maxWeeklyLoss, 2));
         weeklyLimitReached = true;
         CloseAllPositions("Weekly limit");
         if(SendAlerts) Alert("Weekly loss limit reached!");
      }
      return false;
   }

   if(UseDrawdownProtection)
   {
      double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);
      double drawdown = ((peakBalance - currentBalance) / peakBalance) * 100;

      if(drawdown > maxDrawdownReached) maxDrawdownReached = drawdown;

      if(drawdown >= MaxDrawdown)
      {
         Print("⛔ MAX DRAWDOWN REACHED!");
         Print("   Drawdown: ", DoubleToString(drawdown, 2), "%");
         emergencyStop = true;
         CloseAllPositions("Drawdown limit");
         if(SendAlerts) Alert("Max drawdown reached!");
         return false;
      }
   }

   if(consecutiveLosses >= MaxConsecutiveLosses)
   {
      if(!emergencyStop)
      {
         Print("🚨 EMERGENCY STOP!");
         Print("   Consecutive Losses: ", consecutiveLosses);
         emergencyStop = true;
         CloseAllPositions("Emergency stop");
         if(SendAlerts) Alert("Emergency stop triggered!");
      }
      return false;
   }

   return true;
}

//+------------------------------------------------------------------+
//| Check spread acceptability                                        |
//+------------------------------------------------------------------+
bool IsSpreadAcceptable()
{
   if(atr[0] <= 0) return false;

   double spread = SymbolInfoInteger(_Symbol, SYMBOL_SPREAD) * _Point;
   double maxSpread;

   if(MaxSpreadPips > 0)
   {
      maxSpread = MaxSpreadPips * _Point * 10;
   }
   else
   {
      maxSpread = atr[0] * MaxSpreadATR;
   }

   if(spread > maxSpread)
   {
      datetime currentTime = TimeCurrent();
      if(currentTime - lastSpreadWarning > 300)
      {
         Print("⚠️ Spread too high: ", DoubleToString(spread / _Point, 1), " points");
         Print("   Max allowed: ", DoubleToString(maxSpread / _Point, 1), " points");
         lastSpreadWarning = currentTime;
      }
      return false;
   }

   return true;
}

//+------------------------------------------------------------------+
//| ⭐ FIXED: Volatility filter with tolerance                        |
//+------------------------------------------------------------------+
bool IsVolatilityAcceptable()
{
   if(atr[0] <= 0) return false;

   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);
   if(currentPrice <= 0) return false;

   double volatilityRatio = (atr[0] / currentPrice) * 100.0;

   // ⭐ FIXED: Add tolerance for floating-point comparison
   if(volatilityRatio < (MinVolatility - VOLATILITY_TOLERANCE))
   {
      return false;
   }

   if(volatilityRatio > (MaxVolatility + VOLATILITY_TOLERANCE))
   {
      return false;
   }

   return true;
}

//+------------------------------------------------------------------+
//| Check if in trading session                                       |
//+------------------------------------------------------------------+
bool IsTradingSession()
{
   MqlDateTime time;
   TimeToStruct(TimeCurrent(), time);

   if(AvoidWeekends)
   {
      if(time.day_of_week == 0 && time.hour < SundayOpenHour) return false;
      if(time.day_of_week == 5 && time.hour >= FridayCloseHour) return false;
   }

   bool inSession = false;

   if(TradeAsianSession && time.hour >= 23 || time.hour < 8)
      inSession = true;

   if(TradeLondonSession && time.hour >= 7 && time.hour < 16)
      inSession = true;

   if(TradeNYSession && time.hour >= 12 && time.hour < 21)
      inSession = true;

   return inSession;
}

//+------------------------------------------------------------------+
//| Check and execute trading signals                                 |
//+------------------------------------------------------------------+
void CheckAndExecuteSignals()
{
   int longSignals = 0, shortSignals = 0;
   string longReasons = "", shortReasons = "";

   if(UseTrendStrategy)
   {
      if(CheckTrendLong())
      {
         longSignals++;
         longReasons += "TREND ";
      }
      if(CheckTrendShort())
      {
         shortSignals++;
         shortReasons += "TREND ";
      }
   }

   if(UseBreakoutStrategy)
   {
      if(CheckBreakoutLong())
      {
         longSignals++;
         longReasons += "BREAKOUT ";
      }
      if(CheckBreakoutShort())
      {
         shortSignals++;
         shortReasons += "BREAKOUT ";
      }
   }

   if(UseMomentumStrategy)
   {
      if(CheckMomentumLong())
      {
         longSignals++;
         longReasons += "MOMENTUM ";
      }
      if(CheckMomentumShort())
      {
         shortSignals++;
         shortReasons += "MOMENTUM ";
      }
   }

   if(UseStructureStrategy)
   {
      if(CheckStructureLong())
      {
         longSignals++;
         longReasons += "STRUCTURE ";
      }
      if(CheckStructureShort())
      {
         shortSignals++;
         shortReasons += "STRUCTURE ";
      }
   }

   if(UseMultiTimeframe)
   {
      if(!CheckHigherTimeframeAlignment(ORDER_TYPE_BUY))
         longSignals = 0;
      if(!CheckHigherTimeframeAlignment(ORDER_TYPE_SELL))
         shortSignals = 0;
   }

   int requiredSignals = RequireMultipleSignals ? 2 : 1;

   if(longSignals >= requiredSignals)
   {
      Print("🟢 LONG SIGNAL DETECTED");
      Print("   Signals: ", longSignals, " | ", longReasons);
      Print("   Price: ", DoubleToString(SymbolInfoDouble(_Symbol, SYMBOL_ASK), _Digits));
      Print("   ADX: ", DoubleToString(adx[0], 1));
      Print("   RSI: ", DoubleToString(rsi[0], 1));

      double sl = CalculateStopLoss(ORDER_TYPE_BUY);
      double tp = CalculateDynamicTakeProfit(ORDER_TYPE_BUY, sl);

      if(ValidateRiskReward(sl, tp, ORDER_TYPE_BUY))
      {
         double lotSize = CalculateLotSize(sl);
         if(lotSize > 0)
         {
            OpenTrade(ORDER_TYPE_BUY, lotSize, sl, tp, longReasons);
         }
      }
      else
      {
         Print("   ❌ R:R validation failed");
      }
   }

   if(shortSignals >= requiredSignals)
   {
      Print("🔴 SHORT SIGNAL DETECTED");
      Print("   Signals: ", shortSignals, " | ", shortReasons);
      Print("   Price: ", DoubleToString(SymbolInfoDouble(_Symbol, SYMBOL_BID), _Digits));
      Print("   ADX: ", DoubleToString(adx[0], 1));
      Print("   RSI: ", DoubleToString(rsi[0], 1));

      double sl = CalculateStopLoss(ORDER_TYPE_SELL);
      double tp = CalculateDynamicTakeProfit(ORDER_TYPE_SELL, sl);

      if(ValidateRiskReward(sl, tp, ORDER_TYPE_SELL))
      {
         double lotSize = CalculateLotSize(sl);
         if(lotSize > 0)
         {
            OpenTrade(ORDER_TYPE_SELL, lotSize, sl, tp, shortReasons);
         }
      }
      else
      {
         Print("   ❌ R:R validation failed");
      }
   }
}

//+------------------------------------------------------------------+
//| Trend following strategy - LONG                                   |
//+------------------------------------------------------------------+
bool CheckTrendLong()
{
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);

   bool emaAligned = (ema_fast[0] > ema_medium[0] &&
                      ema_medium[0] > ema_slow[0] &&
                      ema_slow[0] > ema_trend[0]);

   bool priceAboveTrend = (currentPrice > ema_fast[0]);

   bool rsiHealthy = (rsi[0] > 50 && rsi[0] < RSI_OB);

   bool adxStrong = (adx[0] >= MinADX_Trend);

   bool momentumPositive = (ema_fast[0] > ema_fast[1]);

   return (emaAligned && priceAboveTrend && rsiHealthy && adxStrong && momentumPositive);
}

//+------------------------------------------------------------------+
//| ⭐ FIXED: Trend following strategy - SHORT (Relaxed RSI)          |
//+------------------------------------------------------------------+
bool CheckTrendShort()
{
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

   bool emaAligned = (ema_fast[0] < ema_medium[0] &&
                      ema_medium[0] < ema_slow[0] &&
                      ema_slow[0] < ema_trend[0]);

   bool priceBelowTrend = (currentPrice < ema_fast[0]);

   // ⭐ FIXED: Changed from rsi[0] < 50 to rsi[0] < 55
   // Allows for slightly bullish RSI during bearish trend formation
   bool rsiHealthy = (rsi[0] < 55 && rsi[0] > RSI_OS);

   bool adxStrong = (adx[0] >= MinADX_Trend);

   bool momentumNegative = (ema_fast[0] < ema_fast[1]);

   return (emaAligned && priceBelowTrend && rsiHealthy && adxStrong && momentumNegative);
}

//+------------------------------------------------------------------+
//| Breakout strategy - LONG                                          |
//+------------------------------------------------------------------+
bool CheckBreakoutLong()
{
   double high20 = 0;
   for(int i = 1; i <= 20; i++)
   {
      double h = GetHighPrice(_Symbol, PrimaryTF, i);
      if(h > high20) high20 = h;
   }

   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);
   double breakoutStrength = (currentPrice - high20) / atr[0];

   bool priceBreakout = (currentPrice > high20);
   bool strongBreakout = (breakoutStrength > 0.3);
   bool rsiConfirm = (rsi[0] > 55 && rsi[0] < 75);
   bool adxStrong = (adx[0] >= MinADX_Strong);
   bool volumeExpansion = (atr[0] > atr[1]);

   return (priceBreakout && strongBreakout && rsiConfirm && adxStrong && volumeExpansion);
}

//+------------------------------------------------------------------+
//| Breakout strategy - SHORT                                         |
//+------------------------------------------------------------------+
bool CheckBreakoutShort()
{
   double low20 = 999999;
   for(int i = 1; i <= 20; i++)
   {
      double l = GetLowPrice(_Symbol, PrimaryTF, i);
      if(l < low20) low20 = l;
   }

   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
   double breakoutStrength = (low20 - currentPrice) / atr[0];

   bool priceBreakout = (currentPrice < low20);
   bool strongBreakout = (breakoutStrength > 0.3);
   bool rsiConfirm = (rsi[0] < 45 && rsi[0] > 25);
   bool adxStrong = (adx[0] >= MinADX_Strong);
   bool volumeExpansion = (atr[0] > atr[1]);

   return (priceBreakout && strongBreakout && rsiConfirm && adxStrong && volumeExpansion);
}

//+------------------------------------------------------------------+
//| Momentum strategy - LONG                                          |
//+------------------------------------------------------------------+
bool CheckMomentumLong()
{
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);

   bool fastRising = (ema_fast[0] > ema_fast[1] && ema_fast[1] > ema_fast[2]);

   bool rsiMomentum = (rsi[0] > 60 && rsi[0] > rsi[1]);

   bool aboveTrend = (currentPrice > ema_trend[0]);

   bool bbExpansion = (bb_upper[0] - bb_lower[0]) > (bb_upper[1] - bb_lower[1]);

   bool adxRising = (adx[0] > adx[1]);

   return (fastRising && rsiMomentum && aboveTrend && bbExpansion && adxRising);
}

//+------------------------------------------------------------------+
//| Momentum strategy - SHORT                                         |
//+------------------------------------------------------------------+
bool CheckMomentumShort()
{
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

   bool fastFalling = (ema_fast[0] < ema_fast[1] && ema_fast[1] < ema_fast[2]);

   bool rsiMomentum = (rsi[0] < 40 && rsi[0] < rsi[1]);

   bool belowTrend = (currentPrice < ema_trend[0]);

   bool bbExpansion = (bb_upper[0] - bb_lower[0]) > (bb_upper[1] - bb_lower[1]);

   bool adxRising = (adx[0] > adx[1]);

   return (fastFalling && rsiMomentum && belowTrend && bbExpansion && adxRising);
}

//+------------------------------------------------------------------+
//| Structure strategy - LONG                                         |
//+------------------------------------------------------------------+
bool CheckStructureLong()
{
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);

   bool higherHigh = false;
   double recentHigh = GetHighPrice(_Symbol, PrimaryTF, 1);
   for(int i = 2; i <= 10; i++)
   {
      if(recentHigh > GetHighPrice(_Symbol, PrimaryTF, i))
      {
         higherHigh = true;
         break;
      }
   }

   bool higherLow = false;
   double recentLow = GetLowPrice(_Symbol, PrimaryTF, 1);
   for(int i = 2; i <= 10; i++)
   {
      if(recentLow > GetLowPrice(_Symbol, PrimaryTF, i))
      {
         higherLow = true;
         break;
      }
   }

   bool structureShift = (ema_fast[0] > ema_slow[0] && ema_fast[1] <= ema_slow[1]);

   bool bullishLongTerm = (currentPrice > ema_longterm[0]);

   return ((higherHigh || higherLow || structureShift) && bullishLongTerm);
}

//+------------------------------------------------------------------+
//| Structure strategy - SHORT                                        |
//+------------------------------------------------------------------+
bool CheckStructureShort()
{
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

   bool lowerLow = false;
   double recentLow = GetLowPrice(_Symbol, PrimaryTF, 1);
   for(int i = 2; i <= 10; i++)
   {
      if(recentLow < GetLowPrice(_Symbol, PrimaryTF, i))
      {
         lowerLow = true;
         break;
      }
   }

   bool lowerHigh = false;
   double recentHigh = GetHighPrice(_Symbol, PrimaryTF, 1);
   for(int i = 2; i <= 10; i++)
   {
      if(recentHigh < GetHighPrice(_Symbol, PrimaryTF, i))
      {
         lowerHigh = true;
         break;
      }
   }

   bool structureShift = (ema_fast[0] < ema_slow[0] && ema_fast[1] >= ema_slow[1]);

   bool bearishLongTerm = (currentPrice < ema_longterm[0]);

   return ((lowerLow || lowerHigh || structureShift) && bearishLongTerm);
}

//+------------------------------------------------------------------+
//| Check higher timeframe alignment                                  |
//+------------------------------------------------------------------+
bool CheckHigherTimeframeAlignment(ENUM_ORDER_TYPE orderType)
{
   if(!UseMultiTimeframe) return true;

   if(orderType == ORDER_TYPE_BUY)
   {
      bool htfTrend = (ema_fast_htf[0] > ema_slow_htf[0]);
      bool htfRSI = (rsi_htf[0] > 50);
      return (htfTrend && htfRSI);
   }
   else
   {
      bool htfTrend = (ema_fast_htf[0] < ema_slow_htf[0]);
      bool htfRSI = (rsi_htf[0] < 50);
      return (htfTrend && htfRSI);
   }
}

//+------------------------------------------------------------------+
//| Calculate stop loss                                               |
//+------------------------------------------------------------------+
double CalculateStopLoss(ENUM_ORDER_TYPE orderType)
{
   double currentPrice = (orderType == ORDER_TYPE_BUY) ?
                         SymbolInfoDouble(_Symbol, SYMBOL_ASK) :
                         SymbolInfoDouble(_Symbol, SYMBOL_BID);

   double atrMultiplier = ATR_SL_NORMAL;
   double volatility = GetVolatilityRegime();

   if(volatility >= VOLATILITY_EXTREME)
      atrMultiplier = ATR_SL_WIDE;
   else if(volatility >= VOLATILITY_VERY_HIGH)
      atrMultiplier = ATR_SL_NORMAL;
   else
      atrMultiplier = ATR_SL_TIGHT;

   double slDistance = atr[0] * atrMultiplier;
   double sl = (orderType == ORDER_TYPE_BUY) ?
               currentPrice - slDistance :
               currentPrice + slDistance;

   return NormalizeDouble(sl, _Digits);
}

//+------------------------------------------------------------------+
//| Get volatility regime                                             |
//+------------------------------------------------------------------+
double GetVolatilityRegime()
{
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);
   return (atr[0] / currentPrice) * 100;
}

//+------------------------------------------------------------------+
//| Calculate dynamic take profit                                     |
//+------------------------------------------------------------------+
double CalculateDynamicTakeProfit(ENUM_ORDER_TYPE orderType, double stopLoss)
{
   double currentPrice = (orderType == ORDER_TYPE_BUY) ?
                         SymbolInfoDouble(_Symbol, SYMBOL_ASK) :
                         SymbolInfoDouble(_Symbol, SYMBOL_BID);

   double slDistance = MathAbs(currentPrice - stopLoss);
   double rrMultiplier = BaseRR;

   if(UseDynamicTP)
   {
      double trendStrength = CalculateTrendStrength();
      double momentum = CalculateMomentum();
      double volatility = GetVolatilityRegime();

      if(trendStrength > TREND_STRENGTH_EXTREME && momentum > MOMENTUM_EXTREME)
         rrMultiplier = MaxRR;
      else if(trendStrength > TREND_STRENGTH_VERY_HIGH && momentum > MOMENTUM_VERY_STRONG)
         rrMultiplier = BaseRR + ((MaxRR - BaseRR) * 0.80);
      else if(trendStrength > TREND_STRENGTH_HIGH && momentum > MOMENTUM_STRONG)
         rrMultiplier = BaseRR + ((MaxRR - BaseRR) * 0.60);
      else if(trendStrength > TREND_STRENGTH_MEDIUM)
         rrMultiplier = BaseRR + ((MaxRR - BaseRR) * 0.40);

      if(volatility >= VOLATILITY_EXTREME)
         rrMultiplier *= 1.2;
      else if(volatility <= VOLATILITY_HIGH)
         rrMultiplier *= 0.9;

      rrMultiplier = MathMax(BaseRR, MathMin(rrMultiplier, MaxRR));
   }

   double tpDistance = slDistance * rrMultiplier;
   double tp = (orderType == ORDER_TYPE_BUY) ?
               currentPrice + tpDistance :
               currentPrice - tpDistance;

   return NormalizeDouble(tp, _Digits);
}

//+------------------------------------------------------------------+
//| Calculate trend strength                                          |
//+------------------------------------------------------------------+
double CalculateTrendStrength()
{
   double score = 0.0;

   bool bullAlign = (ema_fast[0] > ema_medium[0] && ema_medium[0] > ema_slow[0] && ema_slow[0] > ema_trend[0]);
   bool bearAlign = (ema_fast[0] < ema_medium[0] && ema_medium[0] < ema_slow[0] && ema_slow[0] < ema_trend[0]);

   if(bullAlign || bearAlign) score += 0.4;

   double separation = MathAbs(ema_fast[0] - ema_slow[0]) / atr[0];
   if(separation > 3.0) score += 0.3;
   else if(separation > 2.0) score += 0.2;
   else if(separation > 1.0) score += 0.1;

   if(adx[0] > MinADX_Strong) score += 0.3;
   else if(adx[0] > MinADX_Trend) score += 0.2;

   return MathMin(score, 1.0);
}

//+------------------------------------------------------------------+
//| Calculate momentum                                                |
//+------------------------------------------------------------------+
double CalculateMomentum()
{
   double score = 0.0;

   if(rsi[0] > 70 || rsi[0] < 30) score += 0.4;
   else if(rsi[0] > 60 || rsi[0] < 40) score += 0.3;
   else if(rsi[0] > 55 || rsi[0] < 45) score += 0.2;

   if(MathAbs(rsi[0] - rsi[1]) > 3) score += 0.2;

   double priceChange = MathAbs(GetClosePrice(_Symbol, PrimaryTF, 0) - GetClosePrice(_Symbol, PrimaryTF, 1));
   double avgChange = atr[0] * 0.5;

   if(priceChange > avgChange * 1.5) score += 0.4;
   else if(priceChange > avgChange) score += 0.2;

   return MathMin(score, 1.0);
}

//+------------------------------------------------------------------+
//| Validate risk:reward ratio                                        |
//+------------------------------------------------------------------+
bool ValidateRiskReward(double sl, double tp, ENUM_ORDER_TYPE orderType)
{
   double currentPrice = (orderType == ORDER_TYPE_BUY) ?
                         SymbolInfoDouble(_Symbol, SYMBOL_ASK) :
                         SymbolInfoDouble(_Symbol, SYMBOL_BID);

   double slDistance = MathAbs(currentPrice - sl);
   double tpDistance = MathAbs(tp - currentPrice);

   if(slDistance == 0) return false;

   double rrRatio = tpDistance / slDistance;

   if(rrRatio < MinRiskReward)
   {
      Print("   R:R too low: ", DoubleToString(rrRatio, 2), ":1");
      return false;
   }

   return true;
}

//+------------------------------------------------------------------+
//| Calculate lot size                                                |
//+------------------------------------------------------------------+
double CalculateLotSize(double stopLoss)
{
   if(FixedLot > 0) return MathMin(FixedLot, MaxLotSize);

   double accountBalance = AccountInfoDouble(ACCOUNT_BALANCE);
   double riskAmount = accountBalance * (RiskPercent / 100);

   if(UseVolatilityScaling)
   {
      double volatility = GetVolatilityRegime();
      if(volatility >= VOLATILITY_EXTREME)
         riskAmount *= 0.7;
      else if(volatility >= VOLATILITY_VERY_HIGH)
         riskAmount *= 0.85;
   }

   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
   double slDistance = MathAbs(currentPrice - stopLoss);

   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
   double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

   if(tickValue == 0 || tickSize == 0 || slDistance == 0) return 0;

   double lotSize = riskAmount / ((slDistance / tickSize) * tickValue);

   double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
   double maxLot = MathMin(SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX), MaxLotSize);
   double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);

   if(lotStep == 0) lotStep = 0.01;

   lotSize = MathFloor(lotSize / lotStep) * lotStep;
   lotSize = MathMax(minLot, MathMin(maxLot, lotSize));

   return NormalizeDouble(lotSize, 2);
}

//+------------------------------------------------------------------+
//| Open trade                                                         |
//+------------------------------------------------------------------+
void OpenTrade(ENUM_ORDER_TYPE orderType, double lotSize, double sl, double tp, string reason)
{
   MqlTradeRequest request = {};
   MqlTradeResult result = {};

   request.action = TRADE_ACTION_DEAL;
   request.symbol = _Symbol;
   request.volume = lotSize;
   request.type = orderType;
   request.price = (orderType == ORDER_TYPE_BUY) ?
                   SymbolInfoDouble(_Symbol, SYMBOL_ASK) :
                   SymbolInfoDouble(_Symbol, SYMBOL_BID);
   request.sl = sl;
   request.tp = tp;
   request.deviation = Slippage;
   request.magic = MagicNumber;
   request.comment = TradeComment;

   if(OrderSend(request, result))
   {
      double rr = MathAbs(tp - result.price) / MathAbs(result.price - sl);

      Print("╔══════════════════════════════════════════════════════╗");
      Print("║  ✅ TRADE OPENED #", result.order);
      Print("╚══════════════════════════════════════════════════════╝");
      Print("Direction: ", (orderType == ORDER_TYPE_BUY ? "🟢 LONG" : "🔴 SHORT"));
      Print("Entry: ", DoubleToString(result.price, _Digits));
      Print("SL: ", DoubleToString(sl, _Digits), " (-", DoubleToString(MathAbs(result.price - sl), _Digits), ")");
      Print("TP: ", DoubleToString(tp, _Digits), " (+", DoubleToString(MathAbs(tp - result.price), _Digits), ")");
      Print("Lot Size: ", lotSize);
      Print("R:R Ratio: ", DoubleToString(rr, 2), ":1");
      Print("Risk: $", DoubleToString(AccountInfoDouble(ACCOUNT_BALANCE) * RiskPercent / 100, 2));
      Print("Strategies: ", reason);
      Print("══════════════════════════════════════════════════════");

      CreatePositionTracking(result.order, lotSize, result.price, reason);
      dailyTradeCount++;
      totalTrades++;
      lastTradeTime = TimeCurrent();

      if(SendAlerts)
      {
         string alertMsg = StringFormat("%s Trade #%d | %s @ %s | R:R %.2f:1",
            (orderType == ORDER_TYPE_BUY ? "LONG" : "SHORT"),
            result.order, reason, DoubleToString(result.price, _Digits), rr);
         Alert(alertMsg);
      }

      if(SendPushNotifications)
      {
         SendNotification(StringFormat("BTC %s opened @ %s",
            (orderType == ORDER_TYPE_BUY ? "LONG" : "SHORT"),
            DoubleToString(result.price, _Digits)));
      }
   }
   else
   {
      Print("❌ TRADE FAILED");
      Print("   Error: ", result.comment);
      Print("   Code: ", result.retcode);
   }
}

//+------------------------------------------------------------------+
//| Create position tracking                                          |
//+------------------------------------------------------------------+
void CreatePositionTracking(ulong ticket, double volume, double entryPrice, string reason)
{
   int size = ArraySize(positionTracking);
   ArrayResize(positionTracking, size + 1);

   positionTracking[size].ticket = ticket;
   positionTracking[size].tp1_hit = false;
   positionTracking[size].tp2_hit = false;
   positionTracking[size].tp3_hit = false;
   positionTracking[size].tp4_hit = false;
   positionTracking[size].be_moved = false;
   positionTracking[size].trailing_active = false;
   positionTracking[size].original_volume = volume;
   positionTracking[size].current_volume = volume;
   positionTracking[size].entry_price = entryPrice;
   positionTracking[size].highest_price = entryPrice;
   positionTracking[size].lowest_price = entryPrice;
   positionTracking[size].entry_time = TimeCurrent();
   positionTracking[size].entry_reason = reason;
   positionTracking[size].entry_atr = atr[0];
   positionTracking[size].max_favorable_excursion = 0;
   positionTracking[size].max_adverse_excursion = 0;
}

//+------------------------------------------------------------------+
//| Manage dynamic positions                                          |
//+------------------------------------------------------------------+
void ManageDynamicPositions()
{
   for(int i = PositionsTotal() - 1; i >= 0; i--)
   {
      ulong ticket = PositionGetTicket(i);
      if(ticket <= 0) continue;

      if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
      if(PositionGetInteger(POSITION_MAGIC) != MagicNumber) continue;

      double currentPrice = PositionGetDouble(POSITION_PRICE_CURRENT);
      double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);
      double sl = PositionGetDouble(POSITION_SL);
      double tp = PositionGetDouble(POSITION_TP);
      double volume = PositionGetDouble(POSITION_VOLUME);
      double profit = PositionGetDouble(POSITION_PROFIT);

      ENUM_POSITION_TYPE posType = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

      int trackIndex = FindPositionTrackingIndex(ticket);
      if(trackIndex < 0)
      {
         CreatePositionTracking(ticket, volume, openPrice, "Legacy");
         trackIndex = ArraySize(positionTracking) - 1;
      }

      if(posType == POSITION_TYPE_BUY)
      {
         if(currentPrice > positionTracking[trackIndex].highest_price)
            positionTracking[trackIndex].highest_price = currentPrice;

         double favorableMove = currentPrice - openPrice;
         if(favorableMove > positionTracking[trackIndex].max_favorable_excursion)
            positionTracking[trackIndex].max_favorable_excursion = favorableMove;
      }
      else
      {
         if(currentPrice < positionTracking[trackIndex].lowest_price)
            positionTracking[trackIndex].lowest_price = currentPrice;

         double favorableMove = openPrice - currentPrice;
         if(favorableMove > positionTracking[trackIndex].max_favorable_excursion)
            positionTracking[trackIndex].max_favorable_excursion = favorableMove;
      }

      double riskDistance = MathAbs(openPrice - sl);
      if(riskDistance == 0) continue;

      double profitDistance = (posType == POSITION_TYPE_BUY) ?
                              currentPrice - openPrice :
                              openPrice - currentPrice;
      double currentRR = profitDistance / riskDistance;

      if(UsePartialTP)
      {
         if(!positionTracking[trackIndex].tp1_hit && currentRR >= TP1_RR)
         {
            double closeVol = NormalizeDouble(positionTracking[trackIndex].original_volume * (TP1_Percent / 100), 2);
            if(closeVol >= SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN))
            {
               if(ClosePartialPosition(ticket, closeVol, posType))
               {
                  positionTracking[trackIndex].tp1_hit = true;
                  positionTracking[trackIndex].current_volume -= closeVol;
                  Print("🎯 TP1 Hit #", ticket, " @ ", DoubleToString(currentRR, 2), "R | Closed ", TP1_Percent, "%");
               }
            }
         }

         if(positionTracking[trackIndex].tp1_hit && !positionTracking[trackIndex].tp2_hit && currentRR >= TP2_RR)
         {
            double closeVol = NormalizeDouble(positionTracking[trackIndex].original_volume * (TP2_Percent / 100), 2);
            if(closeVol >= SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN))
            {
               if(ClosePartialPosition(ticket, closeVol, posType))
               {
                  positionTracking[trackIndex].tp2_hit = true;
                  positionTracking[trackIndex].current_volume -= closeVol;
                  Print("🎯 TP2 Hit #", ticket, " @ ", DoubleToString(currentRR, 2), "R | Closed ", TP2_Percent, "%");
               }
            }
         }

         if(positionTracking[trackIndex].tp2_hit && !positionTracking[trackIndex].tp3_hit && currentRR >= TP3_RR)
         {
            double closeVol = NormalizeDouble(positionTracking[trackIndex].original_volume * (TP3_Percent / 100), 2);
            if(closeVol >= SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN))
            {
               if(ClosePartialPosition(ticket, closeVol, posType))
               {
                  positionTracking[trackIndex].tp3_hit = true;
                  positionTracking[trackIndex].current_volume -= closeVol;
                  Print("🎯 TP3 Hit #", ticket, " @ ", DoubleToString(currentRR, 2), "R | Closed ", TP3_Percent, "%");
               }
            }
         }

         if(positionTracking[trackIndex].tp3_hit && !positionTracking[trackIndex].tp4_hit && currentRR >= TP4_RR)
         {
            double closeVol = positionTracking[trackIndex].current_volume;
            if(closeVol >= SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN))
            {
               if(ClosePartialPosition(ticket, closeVol, posType))
               {
                  positionTracking[trackIndex].tp4_hit = true;
                  Print("🎯 TP4 Hit #", ticket, " @ ", DoubleToString(currentRR, 2), "R | Runner closed!");
               }
            }
         }
      }

      if(MoveToBreakeven && !positionTracking[trackIndex].be_moved)
      {
         double tpDistance = MathAbs(tp - openPrice);
         double triggerDistance = tpDistance * BreakevenTrigger;

         if(profitDistance >= triggerDistance)
         {
            double newSL = (posType == POSITION_TYPE_BUY) ?
                          openPrice + (BreakevenBuffer * _Point) :
                          openPrice - (BreakevenBuffer * _Point);

            if((posType == POSITION_TYPE_BUY && newSL > sl) ||
               (posType == POSITION_TYPE_SELL && newSL < sl))
            {
               if(ModifyPosition(ticket, newSL, tp))
               {
                  positionTracking[trackIndex].be_moved = true;
                  Print("🔒 Breakeven Set #", ticket, " @ ", DoubleToString(newSL, _Digits));
               }
            }
         }
      }

      if(UseTrailing && currentRR >= TrailingStart_RR)
      {
         if(!positionTracking[trackIndex].trailing_active)
         {
            positionTracking[trackIndex].trailing_active = true;
            Print("📍 Trailing Started #", ticket);
         }

         double trailDistance = atr[0] * TrailingDistance_ATR;

         if(currentRR >= 5.0)
            trailDistance = atr[0] * 0.8;
         else if(currentRR >= 4.0)
            trailDistance = atr[0] * 1.0;
         else if(currentRR >= 3.0)
            trailDistance = atr[0] * 1.2;

         if(posType == POSITION_TYPE_BUY)
         {
            double newSL = currentPrice - trailDistance;
            if(newSL > sl && newSL > openPrice)
            {
               ModifyPosition(ticket, newSL, tp);
            }
         }
         else
         {
            double newSL = currentPrice + trailDistance;
            if(newSL < sl && newSL < openPrice)
            {
               ModifyPosition(ticket, newSL, tp);
            }
         }
      }
   }
}

//+------------------------------------------------------------------+
//| Close partial position                                            |
//+------------------------------------------------------------------+
bool ClosePartialPosition(ulong ticket, double volume, ENUM_POSITION_TYPE posType)
{
   MqlTradeRequest request = {};
   MqlTradeResult result = {};

   request.action = TRADE_ACTION_DEAL;
   request.position = ticket;
   request.symbol = _Symbol;
   request.volume = volume;
   request.deviation = Slippage;
   request.magic = MagicNumber;

   if(posType == POSITION_TYPE_BUY)
   {
      request.type = ORDER_TYPE_SELL;
      request.price = SymbolInfoDouble(_Symbol, SYMBOL_BID);
   }
   else
   {
      request.type = ORDER_TYPE_BUY;
      request.price = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
   }

   return OrderSend(request, result);
}

//+------------------------------------------------------------------+
//| Modify position                                                    |
//+------------------------------------------------------------------+
bool ModifyPosition(ulong ticket, double sl, double tp)
{
   MqlTradeRequest request = {};
   MqlTradeResult result = {};

   request.action = TRADE_ACTION_SLTP;
   request.position = ticket;
   request.sl = NormalizeDouble(sl, _Digits);
   request.tp = NormalizeDouble(tp, _Digits);

   return OrderSend(request, result);
}

//+------------------------------------------------------------------+
//| Find position tracking index                                      |
//+------------------------------------------------------------------+
int FindPositionTrackingIndex(ulong ticket)
{
   for(int i = 0; i < ArraySize(positionTracking); i++)
   {
      if(positionTracking[i].ticket == ticket) return i;
   }
   return -1;
}

//+------------------------------------------------------------------+
//| Cleanup position tracking                                         |
//+------------------------------------------------------------------+
void CleanupPositionTracking()
{
   for(int i = ArraySize(positionTracking) - 1; i >= 0; i--)
   {
      if(!PositionSelectByTicket(positionTracking[i].ticket))
      {
         if(HistorySelectByPosition(positionTracking[i].ticket))
         {
            double posProfit = 0;
            for(int j = HistoryDealsTotal() - 1; j >= 0; j--)
            {
               ulong dealTicket = HistoryDealGetTicket(j);
               if(HistoryDealGetInteger(dealTicket, DEAL_POSITION_ID) == positionTracking[i].ticket)
               {
                  posProfit += HistoryDealGetDouble(dealTicket, DEAL_PROFIT);
               }
            }

            if(posProfit > 0)
            {
               winningTrades++;
               totalProfit += posProfit;
               if(posProfit > largestWin) largestWin = posProfit;
               consecutiveWins++;
               consecutiveLosses = 0;
            }
            else if(posProfit < 0)
            {
               losingTrades++;
               totalLoss += posProfit;
               if(posProfit < largestLoss) largestLoss = posProfit;
               consecutiveLosses++;
               consecutiveWins = 0;
            }
         }

         int size = ArraySize(positionTracking);
         for(int j = i; j < size - 1; j++)
         {
            positionTracking[j] = positionTracking[j + 1];
         }
         ArrayResize(positionTracking, size - 1);
      }
   }
}

//+------------------------------------------------------------------+
//| Close all positions                                               |
//+------------------------------------------------------------------+
void CloseAllPositions(string reason)
{
   Print("🚨 CLOSING ALL POSITIONS: ", reason);

   for(int i = PositionsTotal() - 1; i >= 0; i--)
   {
      ulong ticket = PositionGetTicket(i);
      if(ticket <= 0) continue;

      if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
      if(PositionGetInteger(POSITION_MAGIC) != MagicNumber) continue;

      MqlTradeRequest request = {};
      MqlTradeResult result = {};

      ENUM_POSITION_TYPE posType = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

      request.action = TRADE_ACTION_DEAL;
      request.position = ticket;
      request.symbol = _Symbol;
      request.volume = PositionGetDouble(POSITION_VOLUME);
      request.deviation = Slippage;
      request.magic = MagicNumber;

      if(posType == POSITION_TYPE_BUY)
      {
         request.type = ORDER_TYPE_SELL;
         request.price = SymbolInfoDouble(_Symbol, SYMBOL_BID);
      }
      else
      {
         request.type = ORDER_TYPE_BUY;
         request.price = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
      }

      OrderSend(request, result);
   }
}

//+------------------------------------------------------------------+
//| Count open positions                                              |
//+------------------------------------------------------------------+
int CountOpenPositions()
{
   int count = 0;
   for(int i = 0; i < PositionsTotal(); i++)
   {
      ulong ticket = PositionGetTicket(i);
      if(ticket <= 0) continue;

      if(PositionGetString(POSITION_SYMBOL) == _Symbol &&
         PositionGetInteger(POSITION_MAGIC) == MagicNumber)
      {
         count++;
      }
   }
   return count;
}

//+------------------------------------------------------------------+
//| Check daily profit                                                |
//+------------------------------------------------------------------+
void CheckDailyProfit()
{
   MqlDateTime today;
   TimeToStruct(TimeCurrent(), today);

   MqlDateTime lastDay;
   TimeToStruct(lastDayCheck, lastDay);

   if(today.day != lastDay.day || today.mon != lastDay.mon || today.year != lastDay.year)
   {
      dailyProfit = 0;
      startingDailyBalance = AccountInfoDouble(ACCOUNT_BALANCE);
      dailyLimitReached = false;
      dailyTradeCount = 0;
      lastDayCheck = TimeCurrent();

      Print("╔══════════════════════════════════════════════════════╗");
      Print("║           NEW TRADING DAY                            ║");
      Print("╚══════════════════════════════════════════════════════╝");
      Print("Date: ", TimeToString(TimeCurrent(), TIME_DATE));
      Print("Balance: $", DoubleToString(startingDailyBalance, 2));
      Print("══════════════════════════════════════════════════════");
   }

   double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);
   dailyProfit = currentBalance - startingDailyBalance;

   if(currentBalance > peakBalance)
      peakBalance = currentBalance;
}

//+------------------------------------------------------------------+
//| Check weekly profit                                               |
//+------------------------------------------------------------------+
void CheckWeeklyProfit()
{
   MqlDateTime today;
   TimeToStruct(TimeCurrent(), today);

   MqlDateTime lastWeek;
   TimeToStruct(lastWeekCheck, lastWeek);

   if(today.day_of_week == 1 && lastWeek.day_of_week != 1)
   {
      weeklyProfit = 0;
      startingWeeklyBalance = AccountInfoDouble(ACCOUNT_BALANCE);
      weeklyLimitReached = false;
      lastWeekCheck = TimeCurrent();

      Print("╔══════════════════════════════════════════════════════╗");
      Print("║           NEW TRADING WEEK                           ║");
      Print("╚══════════════════════════════════════════════════════╝");
      Print("Week starting: ", TimeToString(TimeCurrent(), TIME_DATE));
      Print("Balance: $", DoubleToString(startingWeeklyBalance, 2));
      Print("══════════════════════════════════════════════════════");
   }

   double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);
   weeklyProfit = currentBalance - startingWeeklyBalance;
}

//+------------------------------------------------------------------+
//| Update dashboard                                                   |
//+------------------------------------------------------------------+
void UpdateDashboard()
{
   if(!ShowDashboard) return;

   if(atr[0] <= 0 || adx[0] <= 0 || rsi[0] <= 0) return;

   int xPos = 20;
   int yPos = 50;
   int lineHeight = 20;
   color textColor = clrWhite;
   color headerColor = clrGold;

   CreateLabel("HPEA_Title", xPos, yPos, "🔶 BTC ULTIMATE v4.03", headerColor, 12, "Arial Bold");
   yPos += lineHeight + 5;

   CreateLabel("HPEA_Balance", xPos, yPos, "Balance: $" + DoubleToString(AccountInfoDouble(ACCOUNT_BALANCE), 2), textColor, 10);
   yPos += lineHeight;

   CreateLabel("HPEA_Equity", xPos, yPos, "Equity: $" + DoubleToString(AccountInfoDouble(ACCOUNT_EQUITY), 2), textColor, 10);
   yPos += lineHeight;

   double dailyPercent = (startingDailyBalance > 0) ? (dailyProfit / startingDailyBalance) * 100 : 0;
   color profitColor = (dailyProfit >= 0) ? clrLime : clrRed;
   CreateLabel("HPEA_DailyPL", xPos, yPos, "Daily P/L: $" + DoubleToString(dailyProfit, 2) + " (" + DoubleToString(dailyPercent, 2) + "%)", profitColor, 10);
   yPos += lineHeight;

   double weeklyPercent = (startingWeeklyBalance > 0) ? (weeklyProfit / startingWeeklyBalance) * 100 : 0;
   CreateLabel("HPEA_WeeklyPL", xPos, yPos, "Weekly P/L: $" + DoubleToString(weeklyProfit, 2) + " (" + DoubleToString(weeklyPercent, 2) + "%)", profitColor, 10);
   yPos += lineHeight + 5;

   CreateLabel("HPEA_StatsHeader", xPos, yPos, "📊 Statistics", headerColor, 10);
   yPos += lineHeight;

   CreateLabel("HPEA_TotalTrades", xPos, yPos, "Total Trades: " + IntegerToString(totalTrades), textColor, 9);
   yPos += lineHeight;

   double winRate = (totalTrades > 0) ? (winningTrades * 100.0 / totalTrades) : 0;
   CreateLabel("HPEA_WinRate", xPos, yPos, "Win Rate: " + IntegerToString(winningTrades) + "/" + IntegerToString(totalTrades) + " (" + DoubleToString(winRate, 1) + "%)", textColor, 9);
   yPos += lineHeight;

   CreateLabel("HPEA_ConsecLoss", xPos, yPos, "Consecutive Losses: " + IntegerToString(consecutiveLosses), (consecutiveLosses >= 2) ? clrOrange : textColor, 9);
   yPos += lineHeight;

   CreateLabel("HPEA_DailyTrades", xPos, yPos, "Daily Trades: " + IntegerToString(dailyTradeCount) + "/" + IntegerToString(MaxDailyTrades), textColor, 9);
   yPos += lineHeight + 5;

   CreateLabel("HPEA_MarketHeader", xPos, yPos, "📈 Market", headerColor, 10);
   yPos += lineHeight;

   CreateLabel("HPEA_ADX", xPos, yPos, "ADX: " + DoubleToString(adx[0], 1), textColor, 9);
   yPos += lineHeight;

   CreateLabel("HPEA_RSI", xPos, yPos, "RSI: " + DoubleToString(rsi[0], 1), textColor, 9);
   yPos += lineHeight;

   double spread = SymbolInfoInteger(_Symbol, SYMBOL_SPREAD) * _Point;
   CreateLabel("HPEA_Spread", xPos, yPos, "Spread: " + DoubleToString(spread / _Point, 1) + " pts", textColor, 9);
   yPos += lineHeight;

   yPos += 5;
   string status = emergencyStop ? "⛔ EMERGENCY STOP" :
                   dailyLimitReached ? "⛔ DAILY LIMIT" :
                   weeklyLimitReached ? "⛔ WEEKLY LIMIT" :
                   "✅ ACTIVE";
   color statusColor = emergencyStop || dailyLimitReached || weeklyLimitReached ? clrRed : clrLime;
   CreateLabel("HPEA_Status", xPos, yPos, status, statusColor, 10, "Arial Bold");

   ChartRedraw();
}

//+------------------------------------------------------------------+
//| Create label                                                       |
//+------------------------------------------------------------------+
void CreateLabel(string name, int x, int y, string text, color clr, int fontSize = 10, string font = "Arial")
{
   if(ObjectFind(0, name) < 0)
   {
      ObjectCreate(0, name, OBJ_LABEL, 0, 0, 0);
      ObjectSetInteger(0, name, OBJPROP_CORNER, CORNER_LEFT_UPPER);
      ObjectSetInteger(0, name, OBJPROP_ANCHOR, ANCHOR_LEFT_UPPER);
      ObjectSetInteger(0, name, OBJPROP_XDISTANCE, x);
      ObjectSetInteger(0, name, OBJPROP_YDISTANCE, y);
      ObjectSetString(0, name, OBJPROP_FONT, font);
      ObjectSetInteger(0, name, OBJPROP_FONTSIZE, fontSize);
   }

   ObjectSetString(0, name, OBJPROP_TEXT, text);
   ObjectSetInteger(0, name, OBJPROP_COLOR, clr);
}

//+------------------------------------------------------------------+
//| Get high price (custom wrapper)                                   |
//+------------------------------------------------------------------+
double GetHighPrice(string symbol, ENUM_TIMEFRAMES timeframe, int index)
{
   double highArray[];
   ArraySetAsSeries(highArray, true);
   if(CopyHigh(symbol, timeframe, index, 1, highArray) <= 0) return 0;
   return highArray[0];
}

//+------------------------------------------------------------------+
//| Get low price (custom wrapper)                                    |
//+------------------------------------------------------------------+
double GetLowPrice(string symbol, ENUM_TIMEFRAMES timeframe, int index)
{
   double lowArray[];
   ArraySetAsSeries(lowArray, true);
   if(CopyLow(symbol, timeframe, index, 1, lowArray) <= 0) return 0;
   return lowArray[0];
}

//+------------------------------------------------------------------+
//| Get close price (custom wrapper)                                  |
//+------------------------------------------------------------------+
double GetClosePrice(string symbol, ENUM_TIMEFRAMES timeframe, int index)
{
   double closeArray[];
   ArraySetAsSeries(closeArray, true);
   if(CopyClose(symbol, timeframe, index, 1, closeArray) <= 0) return 0;
   return closeArray[0];
}

//+------------------------------------------------------------------+
//| END OF EA - HighProfitEA v4.03 Bitcoin Ultimate FIXED Edition    |
//+------------------------------------------------------------------+