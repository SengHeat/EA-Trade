//+------------------------------------------------------------------+
//|                          SmartScalpingBot_v3_Pending.mq5         |
//|           Version B - Safe Trend Scalper + Pending Orders        |
//+------------------------------------------------------------------+
#property copyright "Smart Scalping Bot v3 - Safe Trend Scalper"
#property version   "3.10"
#property strict

//==================== ENUMS =========================================//
enum ENUM_EXECUTION_MODE {
   MODE_INSTANT,           // Market Execution (Standard)
   MODE_PENDING_LIMIT,     // Buy Limit / Sell Limit (Pullback)
   MODE_PENDING_STOP,      // Buy Stop / Sell Stop (Breakout)
   MODE_HYBRID_LIMIT       // Market Trade + 1 Limit Order
};

//==================== INPUT PARAMETERS ==============================//
input group "=== Trading Settings ===";
input ENUM_EXECUTION_MODE ExecutionMode = MODE_INSTANT; // Execution Strategy
input int      MaxPositions = 3;              // Maximum positions per signal
input int      MaxTotalPositions = 20;        // Maximum total (Positions + Pending)
input bool     AllowMultipleSignals = true;   // Allow new signals with open positions

input group "=== Pending Order Settings ===";
input double   PendingDistanceATR = 3.0;      // Distance in ATR for Pending Orders
input int      PendingExpirationMin = 60;     // Pending Order Expiration (Minutes)
input bool     DeletePendingOnOpposite = true;// Delete Pending Orders on opposite signal

// --- NEW DYNAMIC LOT SETTINGS ---
input group "=== Money Management ===";
input bool     UseDynamicLots = true;         // Enable Dynamic Lot Sizing
input double   RiskPercent = 3.0;             // Base Risk % per trade setup
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
input int      MinSignalScore = 5;            // Minimum score to trade

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
input double   DailyLossLimit = 500.0;        // Stop trading for the day if loss reaches this
input double   MaxEquityDrawdown = 0.10;      // Stop trading if equity drawdown > 10%
input int      MaxConsecutiveLosses = 100;    // Stop trading after N consecutive losing trades
input bool     EnableEquityStop = true;       // Enable equity stop
input bool     EnableDailyLossStop = false;   // Enable daily loss stop

input group "=== Volatility & TF Filters ===";
input int      ATR_Period = 14;               // ATR period for stop sizing
input double   ATR_SL_Mult = 2.5;             // SL = ATR * this multiplier
input double   ATR_TP_Mult = 3.5;             // TP = ATR * this multiplier
input ENUM_TIMEFRAMES TrendTF1 = PERIOD_M15; // Trend TF1
input ENUM_TIMEFRAMES TrendTF2 = PERIOD_H1;  // Trend TF2
input int      TrendEMA1 = 50;                // EMA for TrendTF1
input int      TrendEMA2 = 200;               // EMA for TrendTF2

input bool     RequireHigherTFTrend = true;   // Require higher TF trend agreement to trade

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
   // Validate inputs
   if(MaxPositions < 1 || MaxPositions > 10) return(INIT_PARAMETERS_INCORRECT);

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

   // Initialize arrays
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
   Print("Smart Scalping Bot v3.1 - Pending Orders Enabled");
   Print("Mode: ", EnumToString(ExecutionMode));
   Print("Max Positions: ", MaxPositions);
   Print("Max Total (Inc Pending): ", MaxTotalPositions);
   Print("========================================");

   return(INIT_SUCCEEDED);
}

//==================== ON DEINIT ====================================//
void OnDeinit(const int reason) {
   IndicatorRelease(emaFastHandle);
   IndicatorRelease(emaSlowHandle);
   IndicatorRelease(rsiHandle);
   IndicatorRelease(bbHandle);
   IndicatorRelease(macdHandle);
}

//==================== ON TICK ======================================//
void OnTick() {
   datetime currentBarTime = iTime(_Symbol, PERIOD_M1, 0);
   bool isNewBar = (currentBarTime != lastBarTime);
   if(!isNewBar) {
      ManagePositions();
      return;
   }
   lastBarTime = currentBarTime;

   if(!UpdateIndicators()) return;

   SyncPositions();

   if(!IsTradingSession()) { UpdateDisplay(); return; }

   // Risk Checks
   if(EnableEquityStop && CheckEquityStop()) { ManagePositions(); return; }
   if(EnableDailyLossStop && CheckDailyLossStop()) { ManagePositions(); return; }

   // Count Open Positions AND Pending Orders
   int totalActive = CountTotalExposure();

   bool canTrade = false;
   if(totalActive == 0) canTrade = true;
   else if(AllowMultipleSignals && totalActive < MaxTotalPositions) {
      int barsSinceLastSignal = (int)((TimeCurrent() - lastSignalTime) / 60);
      if(barsSinceLastSignal >= MinBarsBetweenSignals) canTrade = true;
   }

   if(canTrade) {
      string signal, strength;
      int score;
      AnalyzeSignal(signal, strength, score);

      if(signal != "HOLD" && score >= MinSignalScore) {
         // Check if we need to clean up opposite pending orders
         if(DeletePendingOnOpposite) DeleteOppositePendingOrders(signal);

         if(totalActive + 1 <= MaxTotalPositions) {
            stats.totalSignals++;
            OpenSmartPositions(signal, strength, score);
            lastSignalTime = TimeCurrent();
            lastSignal = signal;
            lastSignalScore = score;
            if(SendNotifications) SendNotification(StringFormat("Signal: %s %s (Score: %d)", signal, strength, score));
         }
      }
   }

   ManagePositions();
   UpdateDisplay();
}

//==================== UPDATE INDICATORS ============================//
bool UpdateIndicators() {
   if(CopyBuffer(emaFastHandle, 0, 0, 3, emaFast) <= 0) return false;
   if(CopyBuffer(emaSlowHandle, 0, 0, 3, emaSlow) <= 0) return false;
   if(CopyBuffer(rsiHandle, 0, 0, 3, rsi) <= 0) return false;
   if(CopyBuffer(bbHandle, 1, 0, 3, bbUpper) <= 0) return false;
   if(CopyBuffer(bbHandle, 2, 0, 3, bbLower) <= 0) return false;
   if(CopyBuffer(bbHandle, 0, 0, 3, bbMid) <= 0) return false;
   if(CopyBuffer(macdHandle, 0, 0, 3, macdMain) <= 0) return false;
   if(CopyBuffer(macdHandle, 1, 0, 3, macdSignal) <= 0) return false;
   return true;
}

//==================== SYNC POSITIONS ================================//
void SyncPositions() {
   // remove closed
   for(int i = ArraySize(positions)-1; i >= 0; i--) {
      if(!PositionSelectByTicket(positions[i].ticket)) {
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
         ArrayRemove(positions, i, 1);
      }
   }

   // Add new positions (including triggered pending orders)
   for(int i = PositionsTotal()-1; i >= 0; i--) {
      ulong ticket = PositionGetTicket(i);
      if(ticket > 0 && PositionGetString(POSITION_SYMBOL) == _Symbol && PositionGetInteger(POSITION_MAGIC) == MagicNumber) {
         bool found = false;
         for(int j = 0; j < ArraySize(positions); j++) {
            if(positions[j].ticket == ticket) {
               found = true;
               positions[j].sl = PositionGetDouble(POSITION_SL);
               positions[j].tp = PositionGetDouble(POSITION_TP);
               break;
            }
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
            positions[size].strength = "UNKNOWN"; // Can't recover strength from history
            positions[size].beMovedTo = false;
            positions[size].trailingActive = false;
            positions[size].highest = positions[size].entryPrice;
            positions[size].lowest = positions[size].entryPrice;
            positions[size].beThreshold = 0.0003;
            positions[size].trailingStart = 0.0005;
            positions[size].trailingStep = 0.0002;
            positions[size].openTime = (datetime)PositionGetInteger(POSITION_TIME);
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

   // 1. EMA Crossover
   if(emaFast[0] > emaSlow[0] && emaFast[1] <= emaSlow[1]) buyScore += 3;
   else if(emaFast[0] < emaSlow[0] && emaFast[1] >= emaSlow[1]) sellScore += 3;

   // 2. Trend
   if(emaFast[0] > emaSlow[0]) buyScore += 1; else if(emaFast[0] < emaSlow[0]) sellScore += 1;

   // 3. RSI
   if(rsi[0] < RSI_Oversold) buyScore += 2; else if(rsi[0] > RSI_Overbought) sellScore += 2;

   // 4. BB
   double bbW = bbUpper[0] - bbLower[0];
   if(bbW > 0) {
      if((iClose(_Symbol,PERIOD_M1,0)-bbLower[0])/bbW < 0.3) buyScore += 2;
      else if((bbUpper[0]-iClose(_Symbol,PERIOD_M1,0))/bbW < 0.3) sellScore += 2;
   }

   // 5. MACD
   if(macdMain[0] > macdSignal[0]) buyScore += 1; else sellScore += 1;

   // ATR & HTF Filter
   double atr = GetATR(PERIOD_M1, ATR_Period);
   int ht = RequireHigherTFTrend ? GetHigherTFTrend() : 0;

   if(buyScore >= MinSignalScore && buyScore > sellScore) {
      if(RequireHigherTFTrend && ht == -1) { signal="HOLD"; return; }
      if(atr > 0 && atr < 5*_Point) { signal="HOLD"; return; }
      signal = "BUY"; score = buyScore;
   } else if(sellScore >= MinSignalScore && sellScore > buyScore) {
      if(RequireHigherTFTrend && ht == 1) { signal="HOLD"; return; }
      if(atr > 0 && atr < 5*_Point) { signal="HOLD"; return; }
      signal = "SELL"; score = sellScore;
   } else {
      signal = "HOLD"; score = 0;
   }

   strength = GetSignalStrength(score);
}

string GetSignalStrength(int score) {
   if(score >= 9) return "VERY_STRONG";
   if(score >= 7) return "STRONG";
   if(score >= 5) return "MEDIUM";
   return "WEAK";
}

StrategySettings GetStrategySettings(string strength) {
   StrategySettings strat;
   // Simple mapping for brevity
   if(strength == "WEAK") { strat.beTrigger=0.0002; strat.trailingStart=0.0004; strat.trailingStep=0.0001; }
   else if(strength == "MEDIUM") { strat.beTrigger=0.0003; strat.trailingStart=0.0005; strat.trailingStep=0.0002; }
   else { strat.beTrigger=0.0004; strat.trailingStart=0.0006; strat.trailingStep=0.0003; }
   return strat;
}

//==================== OPEN POSITIONS (MODIFIED) ====================//
void OpenSmartPositions(string signal, string strength, int score) {
   StrategySettings strat = GetStrategySettings(strength);
   double atrM1 = GetATR(PERIOD_M1, ATR_Period);
   if(atrM1 <= 0) atrM1 = SymbolInfoDouble(_Symbol, SYMBOL_POINT) * 50;

   double slDist = atrM1 * ATR_SL_Mult;
   double tpDist = atrM1 * ATR_TP_Mult;

   // Determine Lot Size
   double lotToTrade = UseDynamicLots ? GetDynamicLot(slDist, RiskPercent, strength) : FixedBaseLot;
   double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
   double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
   lotToTrade = MathFloor(lotToTrade / lotStep) * lotStep;
   if(lotToTrade < minLot) lotToTrade = minLot;

   // Execution Loop
   int positionsToOpen = (ExecutionMode == MODE_HYBRID_LIMIT) ? 1 : MaxPositions;

   // 1. EXECUTE MARKET ORDERS (If mode is Instant or Hybrid)
   if(ExecutionMode == MODE_INSTANT || ExecutionMode == MODE_HYBRID_LIMIT) {
      for(int i=0; i<positionsToOpen; i++) {
         double price = (signal=="BUY")?SymbolInfoDouble(_Symbol,SYMBOL_ASK):SymbolInfoDouble(_Symbol,SYMBOL_BID);
         double sl = (signal=="BUY") ? price - slDist : price + slDist;
         double tp = (signal=="BUY") ? price + tpDist : price - tpDist;

         if(ValidateStops(price, sl, tp, signal)) {
            string comment = StringFormat("%s%d-%s-Mkt", StringSubstr(strength,0,1), score, signal);
            ulong ticket = OpenOrder(signal, lotToTrade, sl, tp, comment);

            if(ticket > 0) {
               AddToPositionStruct(ticket, signal, price, lotToTrade, sl, tp, strength, i+1, strat);
               Print("✓ Market Trade Opened #", ticket);
            }
         }
      }
   }

   // 2. EXECUTE PENDING ORDERS
   if(ExecutionMode != MODE_INSTANT) {
      // Calculate Pending Entry Price
      double pendingEntry = 0;
      double pendingOffset = atrM1 * PendingDistanceATR;
      ENUM_PENDING_TYPE type;

      // Determine Type and Price
      if(ExecutionMode == MODE_PENDING_LIMIT || ExecutionMode == MODE_HYBRID_LIMIT) {
         // Limit Orders (Pullbacks)
         if(signal == "BUY") { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_ASK) - pendingOffset; type = PENDING_LIMIT; }
         else { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_BID) + pendingOffset; type = PENDING_LIMIT; }
      }
      else { // MODE_PENDING_STOP (Breakouts)
         if(signal == "BUY") { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_ASK) + pendingOffset; type = PENDING_STOP; }
         else { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_BID) - pendingOffset; type = PENDING_STOP; }
      }

      // If Hybrid, we only place 1 pending order as a "layer". Otherwise we place MaxPositions
      int pendingCount = (ExecutionMode == MODE_HYBRID_LIMIT) ? 1 : MaxPositions;

      for(int i=0; i<pendingCount; i++) {
         // Recalculate SL/TP relative to Pending Entry
         double sl = (signal=="BUY") ? pendingEntry - slDist : pendingEntry + slDist;
         double tp = (signal=="BUY") ? pendingEntry + tpDist : pendingEntry - tpDist;

         if(ValidateStops(pendingEntry, sl, tp, signal)) {
            string comment = StringFormat("%s%d-%s-Pnd", StringSubstr(strength,0,1), score, signal);
            ulong orderTicket = PlacePendingOrder(type, lotToTrade, pendingEntry, sl, tp, comment);
            if(orderTicket > 0) Print("✓ Pending Order Placed #", orderTicket, " @ ", pendingEntry);
         }
      }
   }
}

//==================== HELPER: ADD TO STRUCT ========================//
void AddToPositionStruct(ulong ticket, string side, double price, double lot, double sl, double tp, string strength, int level, StrategySettings &strat) {
   int size = ArraySize(positions);
   ArrayResize(positions, size + 1);
   positions[size].ticket = ticket;
   positions[size].side = side;
   positions[size].entryPrice = price;
   positions[size].lotSize = lot;
   positions[size].sl = sl;
   positions[size].tp = tp;
   positions[size].level = level;
   positions[size].strength = strength;
   positions[size].beMovedTo = false;
   positions[size].highest = price;
   positions[size].lowest = price;
   positions[size].beThreshold = strat.beTrigger;
   positions[size].trailingStart = strat.trailingStart;
   positions[size].trailingStep = strat.trailingStep;
   positions[size].openTime = TimeCurrent();
}

//==================== OPEN MARKET ORDER ============================//
ulong OpenOrder(string side, double lot, double sl, double tp, string comment) {
   MqlTradeRequest request; MqlTradeResult result;
   ZeroMemory(request); ZeroMemory(result);
   request.action = TRADE_ACTION_DEAL;
   request.symbol = _Symbol;
   request.volume = lot;
   request.type = (side == "BUY") ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
   request.price = (side == "BUY") ? SymbolInfoDouble(_Symbol,SYMBOL_ASK) : SymbolInfoDouble(_Symbol,SYMBOL_BID);
   request.sl = sl; request.tp = tp; request.deviation = Slippage; request.magic = MagicNumber; request.comment = comment;

   if(!OrderSend(request, result)) return 0;
   return result.order;
}

//==================== PLACE PENDING ORDER ==========================//
enum ENUM_PENDING_TYPE { PENDING_LIMIT, PENDING_STOP };

ulong PlacePendingOrder(ENUM_PENDING_TYPE pType, double lot, double price, double sl, double tp, string comment) {
   MqlTradeRequest request; MqlTradeResult result;
   ZeroMemory(request); ZeroMemory(result);

   request.action = TRADE_ACTION_PENDING;
   request.symbol = _Symbol;
   request.volume = lot;
   request.price  = NormalizeDouble(price, _Digits);
   request.sl     = NormalizeDouble(sl, _Digits);
   request.tp     = NormalizeDouble(tp, _Digits);
   request.deviation = Slippage;
   request.magic  = MagicNumber;
   request.comment = comment;

   // Set Type
   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID); // rough check
   if(pType == PENDING_LIMIT) {
       // Buy Limit must be below price, Sell Limit must be above
       if(price < currentPrice) request.type = ORDER_TYPE_BUY_LIMIT;
       else request.type = ORDER_TYPE_SELL_LIMIT;
   } else {
       // Buy Stop must be above, Sell Stop must be below
       if(price > currentPrice) request.type = ORDER_TYPE_BUY_STOP;
       else request.type = ORDER_TYPE_SELL_STOP;
   }

   // Expiration
   if(PendingExpirationMin > 0) {
      request.type_time = ORDER_TIME_SPECIFIED;
      request.expiration = TimeCurrent() + (PendingExpirationMin * 60);
   } else {
      request.type_time = ORDER_TIME_GTC;
   }

   if(!OrderSend(request, result)) {
      Print("Pending Order Error: ", GetLastError(), " ", result.comment);
      return 0;
   }
   return result.order;
}

//==================== DELETE OPPOSITE PENDING ======================//
void DeleteOppositePendingOrders(string newSignal) {
   for(int i=OrdersTotal()-1; i>=0; i--) {
      ulong ticket = OrderGetTicket(i);
      if(ticket > 0 && OrderGetString(ORDER_SYMBOL)==_Symbol && OrderGetInteger(ORDER_MAGIC)==MagicNumber) {
         long type = OrderGetInteger(ORDER_TYPE);
         bool shouldDelete = false;

         // If New Signal is BUY, delete Sell Limits/Stops
         if(newSignal == "BUY") {
            if(type == ORDER_TYPE_SELL_LIMIT || type == ORDER_TYPE_SELL_STOP) shouldDelete = true;
         }
         // If New Signal is SELL, delete Buy Limits/Stops
         else if(newSignal == "SELL") {
            if(type == ORDER_TYPE_BUY_LIMIT || type == ORDER_TYPE_BUY_STOP) shouldDelete = true;
         }

         if(shouldDelete) {
            MqlTradeRequest request; MqlTradeResult result;
            ZeroMemory(request); ZeroMemory(result);
            request.action = TRADE_ACTION_REMOVE;
            request.order = ticket;
            OrderSend(request, result);
         }
      }
   }
}

//==================== VALIDATE STOPS =================================//
bool ValidateStops(double entryPrice, double &sl, double &tp, string side) {
   int stopLevel = (int)SymbolInfoInteger(_Symbol, SYMBOL_TRADE_STOPS_LEVEL);
   double minDistance = stopLevel * _Point;
   if(minDistance < 5*_Point) minDistance = 5*_Point;
   double spread = SymbolInfoDouble(_Symbol,SYMBOL_ASK) - SymbolInfoDouble(_Symbol,SYMBOL_BID);
   minDistance = MathMax(minDistance, spread * 2.0);

   if(side == "BUY") {
      if(entryPrice - sl < minDistance) sl = entryPrice - minDistance;
      if(tp - entryPrice < minDistance) tp = entryPrice + minDistance;
      if(sl >= entryPrice || tp <= entryPrice) return false;
   } else {
      if(sl - entryPrice < minDistance) sl = entryPrice + minDistance;
      if(entryPrice - tp < minDistance) tp = entryPrice - minDistance;
      if(tp >= entryPrice || sl <= entryPrice) return false;
   }
   sl = NormalizeDouble(sl, _Digits); tp = NormalizeDouble(tp, _Digits);
   return true;
}

//==================== MANAGE POSITIONS (BE & TRAIL) =================//
void ManagePositions() {
   for(int i = ArraySize(positions)-1; i >= 0; i--) {
      if(!PositionSelectByTicket(positions[i].ticket)) continue;

      double currentPrice = (positions[i].side == "BUY") ? SymbolInfoDouble(_Symbol,SYMBOL_BID) : SymbolInfoDouble(_Symbol,SYMBOL_ASK);

      // Track High/Low
      if(positions[i].side == "BUY") { if(currentPrice > positions[i].highest) positions[i].highest = currentPrice; }
      else { if(currentPrice < positions[i].lowest) positions[i].lowest = currentPrice; }

      double profitPct = 0;
      if(positions[i].side == "BUY") profitPct = (currentPrice - positions[i].entryPrice) / positions[i].entryPrice;
      else profitPct = (positions[i].entryPrice - currentPrice) / positions[i].entryPrice;

      // Breakeven
      if(UseBreakeven && !positions[i].beMovedTo && profitPct >= positions[i].beThreshold) {
         double newSL = (positions[i].side == "BUY") ? positions[i].entryPrice * (1 + BreakevenOffset) : positions[i].entryPrice * (1 - BreakevenOffset);
         newSL = NormalizeDouble(newSL, _Digits);
         if(ModifyPosition(positions[i].ticket, newSL, positions[i].tp)) {
            positions[i].sl = newSL; positions[i].beMovedTo = true;
         }
      }

      // Trailing
      if(UseTrailing && positions[i].beMovedTo && profitPct >= positions[i].trailingStart) {
         double trailingSL;
         if(positions[i].side == "BUY") {
            trailingSL = positions[i].highest * (1 - positions[i].trailingStep);
            trailingSL = NormalizeDouble(trailingSL, _Digits);
            if(trailingSL > positions[i].sl + (_Point*5)) {
               if(ModifyPosition(positions[i].ticket, trailingSL, positions[i].tp)) positions[i].sl = trailingSL;
            }
         } else {
            trailingSL = positions[i].lowest * (1 + positions[i].trailingStep);
            trailingSL = NormalizeDouble(trailingSL, _Digits);
            if(trailingSL < positions[i].sl - (_Point*5)) {
               if(ModifyPosition(positions[i].ticket, trailingSL, positions[i].tp)) positions[i].sl = trailingSL;
            }
         }
      }
   }
}

bool ModifyPosition(ulong ticket, double sl, double tp) {
   if(!PositionSelectByTicket(ticket)) return false;
   MqlTradeRequest request; MqlTradeResult result; ZeroMemory(request); ZeroMemory(result);
   request.action = TRADE_ACTION_SLTP; request.symbol = _Symbol; request.position = ticket;
   request.sl = NormalizeDouble(sl,_Digits); request.tp = NormalizeDouble(tp,_Digits); request.magic = MagicNumber;
   if(!OrderSend(request,result)) return false;
   return true;
}

//==================== COUNT ========================================//
int CountTotalExposure() {
   int count = 0;
   // Count Positions
   for(int i=PositionsTotal()-1;i>=0;i--) {
      if(PositionGetTicket(i)>0 && PositionGetString(POSITION_SYMBOL)==_Symbol && PositionGetInteger(POSITION_MAGIC)==MagicNumber) count++;
   }
   // Count Pending Orders
   for(int i=OrdersTotal()-1;i>=0;i--) {
      if(OrderGetTicket(i)>0 && OrderGetString(ORDER_SYMBOL)==_Symbol && OrderGetInteger(ORDER_MAGIC)==MagicNumber) count++;
   }
   return count;
}

//==================== DISPLAY INFO ================================//
void UpdateDisplay() {
   MqlDateTime dt; TimeToStruct(TimeCurrent() + 7*3600, dt);
   double price = iClose(_Symbol, PERIOD_M1, 0);
   int openPos = CountTotalExposure();
   double currentProfit=0;
   for(int i=0;i<ArraySize(positions);i++) {
      if(PositionSelectByTicket(positions[i].ticket)) currentProfit += PositionGetDouble(POSITION_PROFIT);
   }

   string info = StringFormat(
      "SMART SCALPING BOT v3.1 (Pending)\nMode: %s\nTime: %02d:%02d UTC+7\nPrice: %.5f\nPositions+Pending: %d / %d\nLast Signal: %s\nCurrent P/L: $%.2f\nTotal P/L: $%.2f",
      EnumToString(ExecutionMode), dt.hour, dt.min, price, openPos, MaxTotalPositions,
      lastSignal, currentProfit, stats.totalProfit
   );
   Comment(info);
}

//==================== UTILS ========================================//
double GetATR(ENUM_TIMEFRAMES tf, int period) {
   int handle = iATR(_Symbol, tf, period);
   double buffer[]; ArrayResize(buffer,1);
   if(CopyBuffer(handle,0,0,1,buffer) <= 0) { IndicatorRelease(handle); return 0.0; }
   IndicatorRelease(handle); return buffer[0];
}

double GetEMA(ENUM_TIMEFRAMES tf, int period) {
   int h = iMA(_Symbol, tf, period, 0, MODE_EMA, PRICE_CLOSE);
   double buf[]; ArrayResize(buf,1);
   if(CopyBuffer(h,0,0,1,buf) <= 0) { IndicatorRelease(h); return 0.0; }
   IndicatorRelease(h); return buf[0];
}

int GetHigherTFTrend() {
   double ema1 = GetEMA(TrendTF1, TrendEMA1);
   double ema2 = GetEMA(TrendTF2, TrendEMA2);
   double p1 = iClose(_Symbol, TrendTF1, 0);
   double p2 = iClose(_Symbol, TrendTF2, 0);
   int up=0, down=0;
   if(p1 > ema1) up++; else down++;
   if(p2 > ema2) up++; else down++;
   if(up > down) return 1; if(down > up) return -1; return 0;
}

double GetDynamicLot(double slDistancePoints, double riskPct, string strength) {
   if(!UseDynamicLots) return FixedBaseLot;
   double equity = AccountInfoDouble(ACCOUNT_EQUITY);
   double riskMoney = (equity * (riskPct / 100.0)) / (double)MaxPositions;

   // Strength Multiplier
   double mult = 1.0;
   if(strength=="MEDIUM") mult=1.5; else if(strength=="STRONG") mult=2.0; else if(strength=="VERY_STRONG") mult=3.0;
   riskMoney *= mult;

   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
   double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
   if(tickValue <= 0 || tickSize <= 0) return FixedBaseLot;

   double lossPerLot = (slDistancePoints / tickSize) * tickValue;
   if(lossPerLot <= 0) return FixedBaseLot;

   return riskMoney / lossPerLot;
}

// Safety Checks
bool CheckEquityStop() {
   double balance = AccountInfoDouble(ACCOUNT_BALANCE);
   double equity = AccountInfoDouble(ACCOUNT_EQUITY);
   return (balance > 0 && ((balance - equity) / balance) >= MaxEquityDrawdown);
}

bool CheckDailyLossStop() {
   double profit = 0;
   datetime start = (datetime)(TimeCurrent() - (TimeCurrent() % 86400));
   if(HistorySelect(start, TimeCurrent())) {
      for(int i=0; i<HistoryDealsTotal(); i++) {
         ulong ticket = HistoryDealGetTicket(i);
         profit += HistoryDealGetDouble(ticket, DEAL_PROFIT);
      }
   }
   return (profit <= -DailyLossLimit);
}
//+------------------------------------------------------------------+