//+------------------------------------------------------------------+
//|                          SmartScalpingBot_v3_Safe.mq5            |
//|                 Version A - Safe Trend Scalper (ATR + HTF)       |
//+------------------------------------------------------------------+
#property copyright "Smart Scalping Bot v3 - Safe Trend Scalper"
#property version   "3.00"
#property strict

//==================== INPUT PARAMETERS ==============================//
input group "=== Trading Settings ===";
input int      MaxPositions = 3;              // Maximum positions per signal
input int      MaxTotalPositions = 21;         // Maximum total open positions
input bool     AllowMultipleSignals = true;  // Allow new signals with open positions

// --- NEW DYNAMIC LOT SETTINGS ---
input group "=== Money Management ===";
input bool     UseDynamicLots = true;         // Enable Dynamic Lot Sizing
input double   RiskPercent = 6.0;             // Base Risk % per trade setup (at Weak strength)
input double   MaxLotSize = 5.0;              // Maximum allowed lot size safety cap
// Fallback if Dynamic Lots = false
input double   FixedBaseLot = 0.01;

input group "=== Indicator Settings ===";
input int      EMA_Fast = 5;                  // Fast EMA Period (M1)
input int      EMA_Slow = 13;                 // Slow EMA Period (M1)
input int      RSI_Period = 9;                // RSI Period (M1)
input int      BB_Period = 15;                // Bollinger Bands Period (M1)
input double   BB_Deviation = 1.5;            // BB Standard Deviation (M1)
input int      MACD_Fast = 12;                // MACD Fast EMA
input int      MACD_Slow = 26;                // MACD Slow EMA
input int      MACD_Signal = 9;               // MACD Signal Period

input group "=== Signal Thresholds ===";
input double   RSI_Oversold = 40;             // RSI Oversold Level
input double   RSI_Overbought = 60;           // RSI Overbought Level
input int      MinSignalScore = 2;            // Minimum score to trade

input group "=== Risk Management ===";
input int      MagicNumber = 234000;          // Magic Number
input int      Slippage = 20;                 // Slippage in points
input double   BreakevenOffset = 0.0001;      // Breakeven offset (fractional)
input bool     UseBreakeven = true;           // Enable Breakeven
input bool     UseTrailing = true;            // Enable Trailing Stop

input group "=== Trading Hours (Cambodia UTC+7) ===";
input bool     UseAsianSession = true;        // Trade Asian Session (00:00-08:00)
input bool     UseLondonSession = true;       // Trade London Session (08:00-16:00)
input bool     UseNYSession = true;           // Trade NY Session (16:00-24:00)

input group "=== Advanced Settings ===";
input int      MinBarsBetweenSignals = 3;     // Minimum bars between signals
input bool     ShowDebugInfo = true;          // Show debug information
input bool     SendNotifications = false;     // Send push notifications

//==================== SAFETY / ATR / HTF ============================//
input group "=== Risk / Safety Limits ===";
input double   DailyLossLimit = 500.0;        // Stop trading for the day if loss reaches this (account currency)
input double   MaxEquityDrawdown = 0.10;      // Stop trading if equity drawdown > 10% (0.10)
input int      MaxConsecutiveLosses = 100;      // Stop trading after N consecutive losing trades
input bool     EnableEquityStop = true;       // Enable equity stop
input bool     EnableDailyLossStop = false;    // Enable daily loss stop

input group "=== Volatility & TF Filters ===";
input int      ATR_Period = 14;               // ATR period for stop sizing
input double   ATR_SL_Mult = 2.5;             // SL = ATR * this multiplier
input double   ATR_TP_Mult = 3.5;             // TP = ATR * this multiplier
input ENUM_TIMEFRAMES TrendTF1 = PERIOD_M15; // Trend TF1
input ENUM_TIMEFRAMES TrendTF2 = PERIOD_H1;  // Trend TF2
input int      TrendEMA1 = 50;                // EMA for TrendTF1
input int      TrendEMA2 = 200;               // EMA for TrendTF2

input bool     RequireHigherTFTrend = true;   // Require higher TF trend agreement to trade
input bool     DisableMultipleSignalsByDefault = true; // safer default

//--- Safer default override (applied in OnInit)
bool saferDefaultsApplied = false;

//==================== GLOBALS ======================================//
int emaFastHandle = INVALID_HANDLE, emaSlowHandle = INVALID_HANDLE;
int rsiHandle = INVALID_HANDLE, bbHandle = INVALID_HANDLE, macdHandle = INVALID_HANDLE;
double emaFast[], emaSlow[], rsi[], bbUpper[], bbLower[], bbMid[], macdMain[], macdSignal[];
datetime lastBarTime = 0;
datetime lastSignalTime = 0;
string lastSignal = "NONE";
int lastSignalScore = 0;

// Position tracking structure
struct PositionInfo {
   ulong ticket;
   string side;
   double entryPrice;
   double lotSize;
   double sl;
   double tp;
   int level;
   string strength;
   bool beMovedTo;
   bool trailingActive;
   double highest;
   double lowest;
   double beThreshold;
   double trailingStart;
   double trailingStep;
   datetime openTime;
};

PositionInfo positions[];

// Strategy settings struct
struct StrategySettings {
   double tpLevels[3];
   double sl;
   double beTrigger;
   double trailingStart;
   double trailingStep;
};

// Stats
struct Stats {
   int totalSignals;
   int totalTrades;
   int winningTrades;
   int losingTrades;
   double totalProfit;
   int consecutiveLosses;
} stats;

//==================== UTIL: Array Remove ============================//
template<typename T>
void ArrayRemove(T &arr[], int index, int count = 1) {
   int size = ArraySize(arr);
   if(index < 0 || index >= size || count <= 0) return;
   int removeCount = MathMin(count, size - index);
   for(int i = index; i < size - removeCount; i++) arr[i] = arr[i + removeCount];
   ArrayResize(arr, size - removeCount);
}

//==================== ON INIT ======================================//
int OnInit() {
   // Apply safer defaults if user left high-risk values
   if(!saferDefaultsApplied) {
      //if(MaxPositions > 3) MaxPositions = 3;
      //if(MaxTotalPositions > 6) MaxTotalPositions = 6;
      //if(DisableMultipleSignalsByDefault) AllowMultipleSignals = false;
      saferDefaultsApplied = true;
   }

   // Validate inputs
   if(MaxPositions < 1 || MaxPositions > 10) {
      Print("ERROR: MaxPositions must be between 1 and 10");
      return(INIT_PARAMETERS_INCORRECT);
   }
   if(MaxTotalPositions < MaxPositions) {
      Print("ERROR: MaxTotalPositions must be >= MaxPositions");
      return(INIT_PARAMETERS_INCORRECT);
   }

   // Create indicator handles (M1 base)
   emaFastHandle = iMA(_Symbol, PERIOD_M1, EMA_Fast, 0, MODE_EMA, PRICE_CLOSE);
   emaSlowHandle = iMA(_Symbol, PERIOD_M1, EMA_Slow, 0, MODE_EMA, PRICE_CLOSE);
   rsiHandle = iRSI(_Symbol, PERIOD_M1, RSI_Period, PRICE_CLOSE);
   bbHandle = iBands(_Symbol, PERIOD_M1, BB_Period, 0, BB_Deviation, PRICE_CLOSE);
   macdHandle = iMACD(_Symbol, PERIOD_M1, MACD_Fast, MACD_Slow, MACD_Signal, PRICE_CLOSE);

   if(emaFastHandle == INVALID_HANDLE || emaSlowHandle == INVALID_HANDLE ||
      rsiHandle == INVALID_HANDLE || bbHandle == INVALID_HANDLE || macdHandle == INVALID_HANDLE) {
      Print("ERROR: Failed to create indicators!");
      return(INIT_FAILED);
   }

   // Initialize arrays (3 values)
   ArrayResize(emaFast, 3); ArrayResize(emaSlow, 3);
   ArrayResize(rsi, 3); ArrayResize(bbUpper, 3); ArrayResize(bbLower, 3); ArrayResize(bbMid, 3);
   ArrayResize(macdMain, 3); ArrayResize(macdSignal, 3);

   ArraySetAsSeries(emaFast, true); ArraySetAsSeries(emaSlow, true);
   ArraySetAsSeries(rsi, true); ArraySetAsSeries(bbUpper, true); ArraySetAsSeries(bbLower, true);
   ArraySetAsSeries(bbMid, true); ArraySetAsSeries(macdMain, true); ArraySetAsSeries(macdSignal, true);

   // Initialize stats
   stats.totalSignals = 0; stats.totalTrades = 0; stats.winningTrades = 0; stats.losingTrades = 0; stats.totalProfit = 0; stats.consecutiveLosses = 0;

   // Sync existing positions
   SyncPositions();

   Print("========================================");
   Print("Smart Scalping Bot v3.00 - Safe Trend Scalper");
   Print("Symbol: ", _Symbol, " | Timeframe: M1");
   Print("Max Positions per Signal: ", MaxPositions);
   Print("Max Total Positions: ", MaxTotalPositions);
   Print("Multiple Signals: ", (AllowMultipleSignals ? "ENABLED" : "DISABLED"));
   Print("ATR Mult: SL=", DoubleToString(ATR_SL_Mult,2), " TP=", DoubleToString(ATR_TP_Mult,2));
   Print("Require HTF Trend: ", (RequireHigherTFTrend ? "YES" : "NO"));
   Print("DailyLossLimit: ", DoubleToString(DailyLossLimit,2));
   Print("MaxEquityDrawdown: ", DoubleToString(MaxEquityDrawdown,2));
   Print("========================================");

   return(INIT_SUCCEEDED);
}

//==================== ON DEINIT ====================================//
void OnDeinit(const int reason) {
   if(emaFastHandle != INVALID_HANDLE) IndicatorRelease(emaFastHandle);
   if(emaSlowHandle != INVALID_HANDLE) IndicatorRelease(emaSlowHandle);
   if(rsiHandle != INVALID_HANDLE) IndicatorRelease(rsiHandle);
   if(bbHandle != INVALID_HANDLE) IndicatorRelease(bbHandle);
   if(macdHandle != INVALID_HANDLE) IndicatorRelease(macdHandle);

   Print("========================================");
   Print("Smart Scalping Bot v3 stopped - Reason: ", GetDeinitReasonText(reason));
   Print("Total Signals: ", stats.totalSignals, " Total Trades: ", stats.totalTrades);
   if(stats.totalTrades > 0) Print("Win Rate: ", DoubleToString(stats.winningTrades * 100.0 / stats.totalTrades, 2), "%");
   Print("Total Profit: ", DoubleToString(stats.totalProfit,2));
   Print("========================================");
}

string GetDeinitReasonText(int reason) {
   switch(reason) {
      case REASON_PROGRAM: return "Program stopped";
      case REASON_REMOVE: return "EA removed from chart";
      case REASON_RECOMPILE: return "EA recompiled";
      case REASON_CHARTCHANGE: return "Chart symbol/period changed";
      case REASON_CHARTCLOSE: return "Chart closed";
      case REASON_PARAMETERS: return "Input parameters changed";
      case REASON_ACCOUNT: return "Account changed";
      case REASON_TEMPLATE: return "Template changed";
      case REASON_INITFAILED: return "Initialization failed";
      case REASON_CLOSE: return "Terminal closed";
      default: return "Unknown reason";
   }
}

//==================== ON TICK ======================================//
void OnTick() {
   // New bar detection (M1)
   datetime currentBarTime = iTime(_Symbol, PERIOD_M1, 0);
   bool isNewBar = (currentBarTime != lastBarTime);
   if(!isNewBar) {
      ManagePositions();
      return;
   }
   lastBarTime = currentBarTime;

   // Update M1 indicators
   if(!UpdateIndicators()) {
      if(ShowDebugInfo) Print("Failed to update indicators");
      return;
   }

   // Sync
   SyncPositions();

   // Trading session
   if(!IsTradingSession()) {
      UpdateDisplay();
      return;
   }

   // Safety checks: equity & daily loss + consecutive losses
   if(EnableEquityStop && CheckEquityStop()) {
      if(ShowDebugInfo) Print("Equity stop triggered - skipping trades");
      UpdateDisplay();
      ManagePositions();
      return;
   }
   if(EnableDailyLossStop && CheckDailyLossStop()) {
      if(ShowDebugInfo) Print("Daily loss limit reached - skipping trades");
      UpdateDisplay();
      ManagePositions();
      return;
   }
   if(stats.consecutiveLosses >= MaxConsecutiveLosses) {  // REMOVED: && false
      if(ShowDebugInfo) Print("Max consecutive losses (", stats.consecutiveLosses, ") reached - skipping trades");
      UpdateDisplay();
      ManagePositions();
      return;
  }

   // Count open positions
   int openPos = CountOpenPositions();

   bool canTrade = false;
   string reason = "";

   if(openPos == 0) {
      canTrade = true; reason = "No open positions";
   } else if(AllowMultipleSignals && openPos < MaxTotalPositions) {
      int barsSinceLastSignal = (int)((TimeCurrent() - lastSignalTime) / 60);
      if(barsSinceLastSignal >= MinBarsBetweenSignals) {
         canTrade = true; reason = "Multiple signals allowed";
      } else {
         reason = StringFormat("Too soon (wait %d more bars)", MinBarsBetweenSignals - barsSinceLastSignal);
      }
   } else reason = "Max positions reached or multiple signals disabled";

   if(ShowDebugInfo && !canTrade) Print("Cannot trade: ", reason);

   if(canTrade) {
      string signal, strength;
      int score;
      AnalyzeSignal(signal, strength, score);

      if(signal != "HOLD" && score >= MinSignalScore) {
         if(openPos + MaxPositions <= MaxTotalPositions) {
            stats.totalSignals++;
            OpenSmartPositions(signal, strength, score);
            lastSignalTime = TimeCurrent();
            lastSignal = signal;
            lastSignalScore = score;
            if(SendNotifications) SendNotification(StringFormat("Signal: %s %s (Score: %d)", signal, strength, score));
         } else {
            if(ShowDebugInfo) Print("Signal detected but cannot open: Would exceed MaxTotalPositions");
         }
      }
   }

   ManagePositions();
   UpdateDisplay();
}

//==================== UPDATE INDICATORS ============================//
bool UpdateIndicators() {
   if(CopyBuffer(emaFastHandle, 0, 0, 3, emaFast) <= 0) { if(ShowDebugInfo) Print("Failed to copy EMA Fast"); return false; }
   if(CopyBuffer(emaSlowHandle, 0, 0, 3, emaSlow) <= 0) { if(ShowDebugInfo) Print("Failed to copy EMA Slow"); return false; }
   if(CopyBuffer(rsiHandle, 0, 0, 3, rsi) <= 0) { if(ShowDebugInfo) Print("Failed to copy RSI"); return false; }
   if(CopyBuffer(bbHandle, 1, 0, 3, bbUpper) <= 0) { if(ShowDebugInfo) Print("Failed to copy BB Upper"); return false; }
   if(CopyBuffer(bbHandle, 2, 0, 3, bbLower) <= 0) { if(ShowDebugInfo) Print("Failed to copy BB Lower"); return false; }
   if(CopyBuffer(bbHandle, 0, 0, 3, bbMid) <= 0) { if(ShowDebugInfo) Print("Failed to copy BB Mid"); return false; }
   if(CopyBuffer(macdHandle, 0, 0, 3, macdMain) <= 0) { if(ShowDebugInfo) Print("Failed to copy MACD Main"); return false; }
   if(CopyBuffer(macdHandle, 1, 0, 3, macdSignal) <= 0) { if(ShowDebugInfo) Print("Failed to copy MACD Signal"); return false; }
   return true;
}

//==================== SYNC POSITIONS ================================//
void SyncPositions() {
   // remove closed
   for(int i = ArraySize(positions)-1; i >= 0; i--) {
      if(!PositionSelectByTicket(positions[i].ticket)) {
         // compute closed profit for stats
         double profit = 0;
         if(HistorySelectByPosition(positions[i].ticket)) {
            for(int j = HistoryDealsTotal() - 1; j >= 0; j--) {
               ulong dealTicket = HistoryDealGetTicket(j);
               if(HistoryDealGetInteger(dealTicket, DEAL_POSITION_ID) == positions[i].ticket)
                  profit += HistoryDealGetDouble(dealTicket, DEAL_PROFIT);
            }
         }
         stats.totalProfit += profit;
         if(profit > 0) { stats.winningTrades++; stats.consecutiveLosses = 0; }
         else if(profit < 0) { stats.losingTrades++; stats.consecutiveLosses++; }
         if(ShowDebugInfo) Print("Position #", positions[i].ticket, " closed profit: ", DoubleToString(profit,2));
         ArrayRemove(positions, i, 1);
      }
   }

   // add external positions with our magic
   for(int i = PositionsTotal()-1; i >= 0; i--) {
      ulong ticket = PositionGetTicket(i);
      if(ticket > 0) {
         if(PositionGetString(POSITION_SYMBOL) == _Symbol && PositionGetInteger(POSITION_MAGIC) == MagicNumber) {
            bool found = false;
            for(int j = 0; j < ArraySize(positions); j++) {
               if(positions[j].ticket == ticket) { found = true; positions[j].sl = PositionGetDouble(POSITION_SL); positions[j].tp = PositionGetDouble(POSITION_TP); break; }
            }
            if(!found) {
               int size = ArraySize(positions);
               ArrayResize(positions, size + 1);
               positions[size].ticket = ticket;
               positions[size].side = (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) ? "BUY" : "SELL";
               positions[size].entryPrice = PositionGetDouble(POSITION_PRICE_OPEN);
               positions[size].lotSize = PositionGetDouble(POSITION_VOLUME);
               positions[size].sl = PositionGetDouble(POSITION_SL);
               positions[size].tp = PositionGetDouble(POSITION_TP);
               positions[size].level = 1;
               positions[size].strength = "UNKNOWN";
               positions[size].beMovedTo = false;
               positions[size].trailingActive = false;
               positions[size].highest = positions[size].entryPrice;
               positions[size].lowest = positions[size].entryPrice;
               positions[size].beThreshold = 0.0003;
               positions[size].trailingStart = 0.0005;
               positions[size].trailingStep = 0.0002;
               positions[size].openTime = (datetime)PositionGetInteger(POSITION_TIME);
               if(ShowDebugInfo) Print("Added external position #", ticket, " to tracking");
            }
         }
      }
   }
}

//==================== SESSION CHECK ================================//
bool IsTradingSession() {
   MqlDateTime dt; TimeToStruct(TimeCurrent() + 7*3600, dt); // UTC+7
   int hour = dt.hour;
   if(UseAsianSession && hour >= 0 && hour < 8) return true;
   if(UseLondonSession && hour >= 8 && hour < 16) return true;
   if(UseNYSession && hour >= 16 && hour < 24) return true;
   return false;
}

//==================== SIGNAL ANALYSIS ==============================//
void AnalyzeSignal(string &signal, string &strength, int &score) {
   int buyScore = 0, sellScore = 0;

   double close0 = iClose(_Symbol, PERIOD_M1, 0);
   double close1 = iClose(_Symbol, PERIOD_M1, 1);
   double volume0 = (double)iVolume(_Symbol, PERIOD_M1, 0);

   // 1. EMA Crossover (3 points)
   bool emaBullCross = (emaFast[0] > emaSlow[0] && emaFast[1] <= emaSlow[1]);
   bool emaBearCross = (emaFast[0] < emaSlow[0] && emaFast[1] >= emaSlow[1]);
   if(emaBullCross) { buyScore += 3; if(ShowDebugInfo) Print("BUY: EMA Bull Cross (+3)"); }
   else if(emaBearCross) { sellScore += 3; if(ShowDebugInfo) Print("SELL: EMA Bear Cross (+3)"); }

   // 2. EMA Trend Strength (1-2 points)
   double emaDiff = MathAbs(emaFast[0] - emaSlow[0]) / MathMax(0.00001, emaSlow[0]);
   if(emaFast[0] > emaSlow[0]) { int p = (emaDiff < 0.001) ? 1 : 2; buyScore += p; if(ShowDebugInfo) Print("BUY: EMA Trend (+",p,")"); }
   else if(emaFast[0] < emaSlow[0]) { int p = (emaDiff < 0.001) ? 1 : 2; sellScore += p; if(ShowDebugInfo) Print("SELL: EMA Trend (+",p,")"); }

   // 3. RSI
   if(rsi[0] < RSI_Oversold) { buyScore += 2; if(ShowDebugInfo) Print("BUY: RSI Oversold (+2) rsi=", DoubleToString(rsi[0],2)); }
   else if(rsi[0] > RSI_Overbought) { sellScore += 2; if(ShowDebugInfo) Print("SELL: RSI Overbought (+2) rsi=", DoubleToString(rsi[0],2)); }

   // 4. BB proximity
   double bbWidth = bbUpper[0] - bbLower[0];
   if(bbWidth > 0) {
      double closeToLower = (close0 - bbLower[0]) / bbWidth;
      double closeToUpper = (bbUpper[0] - close0) / bbWidth;
      if(closeToLower < 0.3) { buyScore += 2; if(ShowDebugInfo) Print("BUY: Near BB Lower (+2)"); }
      else if(closeToUpper < 0.3) { sellScore += 2; if(ShowDebugInfo) Print("SELL: Near BB Upper (+2)"); }
   }

   // 5. MACD
   double macdDiff = macdMain[0] - macdSignal[0];
   if(macdDiff > 0) { buyScore += 1; if(ShowDebugInfo) Print("BUY: MACD Positive (+1)"); }
   else if(macdDiff < 0) { sellScore += 1; if(ShowDebugInfo) Print("SELL: MACD Negative (+1)"); }

   // 6. Volume confirmation using 20-bar average
double volMA = 0;
int volBars = MathMin(20, iBars(_Symbol, PERIOD_M1));  // ADDED: Safety check
for(int i=0; i<volBars; i++) volMA += (double)iVolume(_Symbol, PERIOD_M1, i);
volMA /= (double)volBars;  // CHANGED: Divide by actual bars counted

if(volMA <= 0) {  // ADDED: Validation
   if(ShowDebugInfo) Print("Invalid volume data - skipping volume filter");
} else {
   double volRatio = volume0 / volMA;
   if(volRatio > 1.2) {
      if(buyScore > sellScore) { buyScore += 1; if(ShowDebugInfo) Print("BUY: High Volume (+1)"); }
      else if(sellScore > buyScore) { sellScore += 1; if(ShowDebugInfo) Print("SELL: High Volume (+1)"); }
   }
}
   // ATR check
   double atr = GetATR(PERIOD_M1, ATR_Period);
   if(ShowDebugInfo) Print("ATR M1=", DoubleToString(atr, _Digits));

   // Higher TF trend check
   int ht = 0;
   if(RequireHigherTFTrend) ht = GetHigherTFTrend(); // 1 up, -1 down, 0 unknown
   if(ShowDebugInfo) Print("Higher TF trend:", ht);

   // Determine final signal with HTF agreement and volatility gate
   if(buyScore >= MinSignalScore && buyScore > sellScore) {
      if(RequireHigherTFTrend && ht == -1) { signal="HOLD"; strength="NONE"; score=0; if(ShowDebugInfo) Print("BUY blocked by HTF trend"); return; }
      if(atr > 0 && atr < (SymbolInfoDouble(_Symbol, SYMBOL_POINT) * 5)) { signal="HOLD"; strength="NONE"; score=0; if(ShowDebugInfo) Print("Blocked: ATR too small"); return; }
      signal = "BUY"; strength = GetSignalStrength(buyScore); score = buyScore; if(ShowDebugInfo) Print("=== FINAL: BUY ===");
   } else if(sellScore >= MinSignalScore && sellScore > buyScore) {
      if(RequireHigherTFTrend && ht == 1) { signal="HOLD"; strength="NONE"; score=0; if(ShowDebugInfo) Print("SELL blocked by HTF trend"); return; }
      if(atr > 0 && atr < (SymbolInfoDouble(_Symbol, SYMBOL_POINT) * 5)) { signal="HOLD"; strength="NONE"; score=0; if(ShowDebugInfo) Print("Blocked: ATR too small"); return; }
      signal = "SELL"; strength = GetSignalStrength(sellScore); score = sellScore; if(ShowDebugInfo) Print("=== FINAL: SELL ===");
   } else {
      signal = "HOLD"; strength="NONE"; score=0;
      if(ShowDebugInfo && (buyScore>0 || sellScore>0)) {
         Print("HOLD - insufficient score | buy=", buyScore, " sell=", sellScore);
      }
   }
}

//==================== SIGNAL HELPERS ===============================//
string GetSignalStrength(int score) {
   if(score >= 9) return "VERY_STRONG";
   if(score >= 7) return "STRONG";
   if(score >= 5) return "MEDIUM";
   return "WEAK";
}

StrategySettings GetStrategySettings(string strength) {
   StrategySettings strat;
   if(strength == "WEAK") {
      strat.tpLevels[0]=0.0003; strat.tpLevels[1]=0.0005; strat.tpLevels[2]=0.0007;
      strat.sl=0.0004; 
      strat.beTrigger=0.0002;  // Move BE early for weak signals (more defensive)
      strat.trailingStart=0.0004; strat.trailingStep=0.0001;
   } else if(strength == "MEDIUM") {
      strat.tpLevels[0]=0.0005; strat.tpLevels[1]=0.0008; strat.tpLevels[2]=0.0012;
      strat.sl=0.0006; 
      strat.beTrigger=0.0004;  // CHANGED: Delay BE slightly (was 0.0003)
      strat.trailingStart=0.0005; strat.trailingStep=0.0002;
   } else if(strength == "STRONG") {
      strat.tpLevels[0]=0.0008; strat.tpLevels[1]=0.0012; strat.tpLevels[2]=0.0018;
      strat.sl=0.0006; 
      strat.beTrigger=0.0008;  // CHANGED: Delay BE more for strong signals (was 0.0004)
      strat.trailingStart=0.0006; strat.trailingStep=0.0003;
   } else { // VERY_STRONG
      strat.tpLevels[0]=0.0010; strat.tpLevels[1]=0.0015; strat.tpLevels[2]=0.0025;
      strat.sl=0.0007; 
      strat.beTrigger=0.0012;  // CHANGED: Delay BE significantly for very strong signals (was 0.0005)
      strat.trailingStart=0.0008; strat.trailingStep=0.0004;
   }
   return strat;
}

//==================== OPEN POSITIONS (DYNAMIC) ====================//
void OpenSmartPositions(string signal, string strength, int score) {
   double price = (signal == "BUY") ? SymbolInfoDouble(_Symbol, SYMBOL_ASK) : SymbolInfoDouble(_Symbol, SYMBOL_BID);
   StrategySettings strat = GetStrategySettings(strength);

   // 1. Calculate ATR and Stop Loss Distance FIRST (needed for lot calc)
   double atrM1 = GetATR(PERIOD_M1, ATR_Period);
   if(atrM1 <= 0) atrM1 = SymbolInfoDouble(_Symbol, SYMBOL_POINT) * 50;

   double slDistance = atrM1 * ATR_SL_Mult;
   double tpDistance = atrM1 * ATR_TP_Mult;

   // 2. Calculate Dynamic Lot based on that SL distance
   // Note: We use the logic that RiskPercent is the BASE risk, and Strength acts as a multiplier inside the function
   double lotToTrade = 0;

   if(UseDynamicLots) {
      // Pass the SL distance to calculate risk
      lotToTrade = GetDynamicLot(slDistance, RiskPercent, strength);
   } else {
      // Use fixed base lot multiplied by strength manually
      lotToTrade = NormalizeDouble(FixedBaseLot * GetLotMultiplier(strength), 2);
   }

   // Validate Lot Limits one last time
   double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
   double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
   lotToTrade = MathFloor(lotToTrade / lotStep) * lotStep;
   if(lotToTrade < minLot) lotToTrade = minLot;

   Print("========================================");
   Print("Opening ", MaxPositions, "x ", signal, " | Strength:", strength);
   Print("Balance: ", DoubleToString(AccountInfoDouble(ACCOUNT_BALANCE), 2));
   Print("Calculated Lot: ", DoubleToString(lotToTrade, 2), " (Dynamic: ", UseDynamicLots, ")");
   Print("SL Dist: ", DoubleToString(slDistance, _Digits));
   Print("========================================");

   int successCount = 0;

   for(int i=0; i<MaxPositions; i++) {
      double sl=0, tp=0;

      // Calculate SL/TP prices
      if(signal == "BUY") {
         sl = price - slDistance;
         tp = price + tpDistance;
      } else {
         sl = price + slDistance;
         tp = price - tpDistance;
      }

      sl = NormalizeDouble(sl, _Digits);
      tp = NormalizeDouble(tp, _Digits);

      // Validate stops (broker min distance, spread)
      if(!ValidateStops(price, sl, tp, signal)) {
         Print("Invalid stops for L", i+1, " - skipping");
         continue;
      }

      string comment = StringFormat("%s%d-%s-L%d", StringSubstr(strength,0,1), score, signal, i+1);

      // Use the calculated lotToTrade
      ulong ticket = OpenOrder(signal, lotToTrade, sl, tp, comment);

      if(ticket > 0) {
         int size = ArraySize(positions);
         ArrayResize(positions, size + 1);
         positions[size].ticket = ticket;
         positions[size].side = signal;
         positions[size].entryPrice = price;
         positions[size].lotSize = lotToTrade;
         positions[size].sl = sl;
         positions[size].tp = tp;
         positions[size].level = i+1;
         positions[size].strength = strength;
         positions[size].beMovedTo = false;
         positions[size].trailingActive = false;
         positions[size].highest = price;
         positions[size].lowest = price;
         positions[size].beThreshold = strat.beTrigger;
         positions[size].trailingStart = strat.trailingStart;
         positions[size].trailingStep = strat.trailingStep;
         positions[size].openTime = TimeCurrent();

         Print("✓ Opened L", i+1, " #", ticket, " | Lot:", DoubleToString(lotToTrade,2));
         successCount++;
         stats.totalTrades++;
      } else {
         Print("✗ Failed to open L", i+1);
      }
      Sleep(200); // Small delay between positions
   }
}

//==================== VALIDATE STOPS =================================//
bool ValidateStops(double entryPrice, double &sl, double &tp, string side) {
   int stopLevel = (int)SymbolInfoInteger(_Symbol, SYMBOL_TRADE_STOPS_LEVEL);
   double minDistance = stopLevel * _Point;
   if(minDistance < 5*_Point) minDistance = 5*_Point;
   double spread = SymbolInfoDouble(_Symbol,SYMBOL_ASK) - SymbolInfoDouble(_Symbol,SYMBOL_BID);
   minDistance = MathMax(minDistance, spread * 2.0);

   bool adjusted = false;
   if(side == "BUY") {
      if(entryPrice - sl < minDistance) { sl = entryPrice - minDistance; adjusted = true; if(ShowDebugInfo) Print("SL adjusted to minDistance:", DoubleToString(sl,_Digits)); }
      if(tp - entryPrice < minDistance) { tp = entryPrice + minDistance; adjusted = true; if(ShowDebugInfo) Print("TP adjusted to minDistance:", DoubleToString(tp,_Digits)); }
      if(sl >= entryPrice || tp <= entryPrice) { Print("ERROR: Invalid BUY stops - SL:", DoubleToString(sl,_Digits)," Entry:",DoubleToString(entryPrice,_Digits)," TP:",DoubleToString(tp,_Digits)); return false; }
   } else {
      if(sl - entryPrice < minDistance) { sl = entryPrice + minDistance; adjusted = true; if(ShowDebugInfo) Print("SL adjusted to minDistance:", DoubleToString(sl,_Digits)); }
      if(entryPrice - tp < minDistance) { tp = entryPrice - minDistance; adjusted = true; if(ShowDebugInfo) Print("TP adjusted to minDistance:", DoubleToString(tp,_Digits)); }
      if(tp >= entryPrice || sl <= entryPrice) { Print("ERROR: Invalid SELL stops - TP:", DoubleToString(tp,_Digits)," Entry:",DoubleToString(entryPrice,_Digits)," SL:",DoubleToString(sl,_Digits)); return false; }
   }
   sl = NormalizeDouble(sl, _Digits); tp = NormalizeDouble(tp, _Digits);
   return true;
}

//==================== OPEN ORDER ===================================//
ulong OpenOrder(string side, double lot, double sl, double tp, string comment) {
   MqlTradeRequest request; MqlTradeResult result;
   ZeroMemory(request); ZeroMemory(result);
   request.action = TRADE_ACTION_DEAL;
   request.symbol = _Symbol;
   request.volume = lot;
   request.type = (side == "BUY") ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
   request.price = (side == "BUY") ? SymbolInfoDouble(_Symbol,SYMBOL_ASK) : SymbolInfoDouble(_Symbol,SYMBOL_BID);
   request.sl = sl; request.tp = tp; request.deviation = Slippage; request.magic = MagicNumber; request.comment = comment;
   request.type_filling = ORDER_FILLING_IOC;

   if(!OrderSend(request, result)) {
      request.type_filling = ORDER_FILLING_FOK;
      if(!OrderSend(request, result)) {
         request.type_filling = ORDER_FILLING_RETURN;
         OrderSend(request, result);
      }
   }

   if(result.retcode != TRADE_RETCODE_DONE) {
      Print("OrderSend failed: ", result.retcode, " - ", result.comment);
      Print("Request: ", comment, " ", DoubleToString(lot,2), " @", DoubleToString(request.price,_Digits));
      return 0;
   }
   return result.order;
}

//==================== MANAGE POSITIONS (BE & TRAIL) =================//
void ManagePositions() {
   for(int i = ArraySize(positions)-1; i >= 0; i--) {
      if(!PositionSelectByTicket(positions[i].ticket)) continue;

      double currentPrice = (positions[i].side == "BUY") ? SymbolInfoDouble(_Symbol,SYMBOL_BID) : SymbolInfoDouble(_Symbol,SYMBOL_ASK);

      if(positions[i].side == "BUY") { if(currentPrice > positions[i].highest) positions[i].highest = currentPrice; }
      else { if(currentPrice < positions[i].lowest) positions[i].lowest = currentPrice; }

      double profitPct = 0;
      if(positions[i].side == "BUY") profitPct = (currentPrice - positions[i].entryPrice) / positions[i].entryPrice;
      else profitPct = (positions[i].entryPrice - currentPrice) / positions[i].entryPrice;

      // Breakeven
      if(UseBreakeven && !positions[i].beMovedTo && profitPct >= positions[i].beThreshold) {
         double newSL = (positions[i].side == "BUY") ? positions[i].entryPrice * (1 + BreakevenOffset) : positions[i].entryPrice * (1 - BreakevenOffset);
         newSL = NormalizeDouble(newSL, _Digits);
         bool shouldMove = (positions[i].side=="BUY" && newSL > positions[i].sl) || (positions[i].side=="SELL" && newSL < positions[i].sl);
         if(shouldMove && ModifyPosition(positions[i].ticket, newSL, positions[i].tp)) {
            positions[i].sl = newSL; positions[i].beMovedTo = true;
            Print("✓ Breakeven set #", positions[i].ticket, " @", DoubleToString(newSL,_Digits));
         }
      }

      // Trailing
      if(UseTrailing && positions[i].beMovedTo && profitPct >= positions[i].trailingStart) {
         double trailingSL; bool shouldModify = false;
         if(positions[i].side == "BUY") {
            trailingSL = positions[i].highest * (1 - positions[i].trailingStep);
            trailingSL = NormalizeDouble(trailingSL, _Digits);
            if(trailingSL > positions[i].sl + (_Point*5)) shouldModify = true;
         } else {
            trailingSL = positions[i].lowest * (1 + positions[i].trailingStep);
            trailingSL = NormalizeDouble(trailingSL, _Digits);
            if(trailingSL < positions[i].sl - (_Point*5)) shouldModify = true;
         }
         if(shouldModify && ModifyPosition(positions[i].ticket, trailingSL, positions[i].tp)) {
            double oldSL = positions[i].sl; positions[i].sl = trailingSL;
            Print("✓ Trailing updated #", positions[i].ticket, " from ", DoubleToString(oldSL,_Digits), " -> ", DoubleToString(trailingSL,_Digits));
         }
      }
   }
}

//==================== MODIFY POSITION ===============================//
bool ModifyPosition(ulong ticket, double sl, double tp) {
   if(!PositionSelectByTicket(ticket)) { Print("Position #", ticket, " not found"); return false; }
   double currentSL = PositionGetDouble(POSITION_SL), currentTP = PositionGetDouble(POSITION_TP);
   if(MathAbs(currentSL - sl) < _Point && MathAbs(currentTP - tp) < _Point) return true;

   MqlTradeRequest request; MqlTradeResult result; ZeroMemory(request); ZeroMemory(result);
   request.action = TRADE_ACTION_SLTP; request.symbol = _Symbol; request.position = ticket;
   request.sl = NormalizeDouble(sl,_Digits); request.tp = NormalizeDouble(tp,_Digits); request.magic = MagicNumber;
   if(!OrderSend(request,result)) { Print("Modify error for #", ticket, ": ", GetLastError()); return false; }
   if(result.retcode != TRADE_RETCODE_DONE) { if(ShowDebugInfo) Print("Modify failed #",ticket,":",result.retcode," - ",result.comment); return false; }
   return true;
}

//==================== COUNT POSITIONS ===============================//
int CountOpenPositions() {
   int count = 0;
   for(int i=PositionsTotal()-1;i>=0;i--) {
      ulong ticket = PositionGetTicket(i);
      if(ticket>0) {
         if(PositionGetString(POSITION_SYMBOL) == _Symbol && PositionGetInteger(POSITION_MAGIC) == MagicNumber) count++;
      }
   }
   return count;
}

//==================== DISPLAY INFO ================================//
void UpdateDisplay() {
   MqlDateTime dt; TimeToStruct(TimeCurrent() + 7*3600, dt);
   string session = "CLOSED"; string sessionColor = "🔴";
   if(dt.hour>=0 && dt.hour<8 && UseAsianSession) { session="ASIAN"; sessionColor="🟢"; }
   else if(dt.hour>=8 && dt.hour<16 && UseLondonSession) { session="LONDON"; sessionColor="🟢"; }
   else if(dt.hour>=16 && dt.hour<24 && UseNYSession) { session="NY"; sessionColor="🟢"; }

   double price = iClose(_Symbol, PERIOD_M1, 0);
   int openPos = CountOpenPositions();
   double currentProfit=0;
   for(int i=0;i<ArraySize(positions);i++) {
      if(PositionSelectByTicket(positions[i].ticket)) currentProfit += PositionGetDouble(POSITION_PROFIT);
   }
   double winRate = (stats.totalTrades>0) ? stats.winningTrades*100.0/stats.totalTrades : 0.0;

   string info = StringFormat(
      "SMART SCALPING BOT v3 (Safe)\nTime: %02d:%02d:%02d (UTC+7)\nSession: %s %s\nPrice: %.5f\nRSI: %.2f\nEMA: %.5f / %.5f\nPositions: %d / %d\nMultiple Signals: %s\nLast Signal: %s (Score: %d)\nCurrent P/L: $%.2f\nTotal P/L: $%.2f\nWin Rate: %.1f%%\nTotal Trades: %d\n",
      dt.hour, dt.min, dt.sec, sessionColor, session, price, rsi[0], emaFast[0], emaSlow[0], openPos, MaxTotalPositions, (AllowMultipleSignals ? "ON":"OFF"),
      lastSignal, lastSignalScore, currentProfit, stats.totalProfit, winRate, stats.totalTrades
   );

   Comment(info);
}

//==================== ATR / HTF HELPERS ============================//
double GetATR(ENUM_TIMEFRAMES tf, int period) {
   int handle = iATR(_Symbol, tf, period);
   if(handle == INVALID_HANDLE) return 0.0;
   double buffer[]; ArrayResize(buffer,2); ArraySetAsSeries(buffer,true);
   if(CopyBuffer(handle,0,0,2,buffer) <= 0) { IndicatorRelease(handle); return 0.0; }
   IndicatorRelease(handle); return buffer[0];
}

double GetEMA(ENUM_TIMEFRAMES tf, int period) {
   int h = iMA(_Symbol, tf, period, 0, MODE_EMA, PRICE_CLOSE);
   if(h == INVALID_HANDLE) return 0.0;
   double buf[]; ArrayResize(buf,2); ArraySetAsSeries(buf,true);
   if(CopyBuffer(h,0,0,2,buf) <= 0) { IndicatorRelease(h); return 0.0; }
   IndicatorRelease(h); return buf[0];
}

// returns 1 for up, -1 for down, 0 unknown / mixed
int GetHigherTFTrend() {
   double ema1 = GetEMA(TrendTF1, TrendEMA1);
   double ema2 = GetEMA(TrendTF2, TrendEMA2);
   if(ema1 == 0 || ema2 == 0) return 0;
   int up=0, down=0;
   double priceTF1 = iClose(_Symbol, TrendTF1, 0);
   double priceTF2 = iClose(_Symbol, TrendTF2, 0);
   if(priceTF1 > ema1) up++; else down++;
   if(priceTF2 > ema2) up++; else down++;
   if(up > down) return 1; if(down > up) return -1; return 0;
}

//==================== SAFETY CHECKS ================================//
bool CheckEquityStop() {
   if(!EnableEquityStop) return false;
   double balance = AccountInfoDouble(ACCOUNT_BALANCE);
   double equity = AccountInfoDouble(ACCOUNT_EQUITY);
   if(balance <= 0) return false;
   double drawdown = (balance - equity) / balance;
   if(drawdown >= MaxEquityDrawdown) {
      Print("Equity drawdown trigger: ", DoubleToString(drawdown,3));
      return true;
   }
   return false;
}

double DateOfDay(datetime when) {
   MqlDateTime dt; TimeToStruct(when, dt);
   dt.hour = 0; dt.min = 0; dt.sec = 0;
   return (double)StructToTime(dt);
}

double GetTodayClosedProfit() {
   datetime from = (datetime)DateOfDay(TimeCurrent());
   double total = 0.0;
   ulong deals = HistoryDealsTotal();
   for(int i = deals - 1; i >= 0; i--) {
      ulong dealTicket = HistoryDealGetTicket(i);
      datetime t = (datetime)HistoryDealGetInteger(dealTicket, DEAL_TIME);
      if(t < from) continue;
      total += HistoryDealGetDouble(dealTicket, DEAL_PROFIT);
   }
   return total;
}

bool CheckDailyLossStop() {
   if(!EnableDailyLossStop) return false;
   double todayProfit = GetTodayClosedProfit();
   if(todayProfit <= -MathAbs(DailyLossLimit)) {
      Print("Daily loss limit reached: ", DoubleToString(todayProfit,2));
      return true;
   }
   return false;
}

//==================== Other helpers ================================//
double GetLotMultiplier(string strength) {
   if(strength == "WEAK") return 1.0;
   if(strength == "MEDIUM") return 1.5;
   if(strength == "STRONG") return 2.0;
   return 3.0; // VERY_STRONG
}

//==================== DYNAMIC LOT CALCULATION ======================//
double GetDynamicLot(double slDistancePoints, double riskPct, string strength) {
   if(!UseDynamicLots) return FixedBaseLot;

   double equity = AccountInfoDouble(ACCOUNT_EQUITY);

   // 1. Calculate Money to Risk
   // We divide risk by MaxPositions because the bot opens 'MaxPositions' trades at once
   double riskMoney = (equity * (riskPct / 100.0)) / (double)MaxPositions;

   // 2. Adjust Risk based on Strength (Weak=1.0, Medium=1.5, Strong=2.0, Very=3.0)
   double strengthMult = GetLotMultiplier(strength);
   riskMoney = riskMoney * strengthMult;

   // 3. Get Tick Value for correct math
   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
   double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

   if(tickValue <= 0 || tickSize <= 0) {
      Print("Error: Invalid TickValue, defaulting to FixedLot");
      return FixedBaseLot;
   }

   // 4. Calculate Lot Size: RiskMoney / (SL_Points * TickValue_Per_Point)
   // Note: slDistance is in price (e.g., 0.0050), we need it in points/ticks for calculation
   double lossPerLot = (slDistancePoints / tickSize) * tickValue;

   if(lossPerLot <= 0) return FixedBaseLot;

   double calculatedLot = riskMoney / lossPerLot;

   // 5. Normalize and Check Limits
   double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
   double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
   double step = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);

   // Round to step
   calculatedLot = MathFloor(calculatedLot / step) * step;

   if(calculatedLot < minLot) calculatedLot = minLot;
   if(calculatedLot > maxLot) calculatedLot = maxLot;
   if(calculatedLot > MaxLotSize) calculatedLot = MaxLotSize;

   return calculatedLot;
}

//==================== Count open positions utility =================//
int CountOpenPositionsWithMagic() { return CountOpenPositions(); }

//==================================================================//
