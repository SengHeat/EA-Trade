//+------------------------------------------------------------------+
//|                          SmartScalpingBot_v3_22.mq5              |
//|           Version B - Safe Trend Scalper + Pending Orders        |
//+------------------------------------------------------------------+
#property copyright "Smart Scalping Bot v3.22 - Dynamic BE"
#property version   "3.22"
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
input ENUM_EXECUTION_MODE ExecutionMode = MODE_INSTANT;
input int      MaxPositions = 3;
input int      MaxTotalPositions = 20;
input bool     AllowMultipleSignals = true;

input group "=== Pending Order Settings ===";
input double   PendingDistanceATR = 3.0;
input int      PendingExpirationMin = 60;
input bool     DeletePendingOnOpposite = true;

input group "=== Money Management ===";
input bool     UseDynamicLots = true;
input double   RiskPercent = 2.0;
input double   MinLotSize = 0.1;
input double   MaxLotSize = 10.0;
input double   FixedBaseLot = 0.1;

input group "=== Indicator Settings ===";
input int      EMA_Fast = 5;
input int      EMA_Slow = 13;
input int      RSI_Period = 9;
input int      BB_Period = 15;
input double   BB_Deviation = 1.5;
input int      MACD_Fast = 12;
input int      MACD_Slow = 26;
input int      MACD_Signal = 9;

input group "=== Signal Thresholds ===";
input double   RSI_Oversold = 40;
input double   RSI_Overbought = 60;
input int      MinSignalScore = 4;            // Reduced from 5 to increase frequency

input group "=== Risk Management ===";
input int      MagicNumber = 234000;
input int      Slippage = 20;
input double   BreakevenOffset = 0.0001;      // Distance profit to lock in (approx 1 pip)
input bool     UseBreakeven = true;
input double   BE_Trigger_PctTP = 30.0;       // NEW: Move to BE at 30% of TP distance
input bool     UseTrailing = true;

input group "=== Trading Hours (Cambodia UTC+7) ===";
input bool     UseAsianSession = true;
input bool     UseLondonSession = true;
input bool     UseNYSession = true;

input group "=== Advanced Settings ===";
input int      MinBarsBetweenSignals = 3;
input bool     ShowDebugInfo = true;
input bool     SendNotifications = false;

input group "=== Risk / Safety Limits ===";
input bool     EnableDailyLossStop = true;
input double   BalanceThreshold = 5000.0;
input double   FixedLossBelowThreshold = 1000.0;
input double   PctLossAboveThreshold = 20.0;

input double   MaxEquityDrawdown = 0.10;
input int      MaxConsecutiveLosses = 100;
input bool     EnableEquityStop = true;

input group "=== Volatility & TF Filters ===";
input int      ATR_Period = 14;
input double   ATR_SL_Mult = 2.5;
input double   ATR_TP_Mult = 3.5;
input ENUM_TIMEFRAMES TrendTF1 = PERIOD_M15;
input ENUM_TIMEFRAMES TrendTF2 = PERIOD_H1;
input int      TrendEMA1 = 50;
input int      TrendEMA2 = 200;

// CHANGED: Set to false by default to increase trade frequency
input bool     RequireHigherTFTrend = false;

//==================== GLOBALS ======================================//
int emaFastHandle = INVALID_HANDLE, emaSlowHandle = INVALID_HANDLE;
int rsiHandle = INVALID_HANDLE, bbHandle = INVALID_HANDLE, macdHandle = INVALID_HANDLE;
double emaFast[], emaSlow[], rsi[], bbUpper[], bbLower[], bbMid[], macdMain[], macdSignal[];
datetime lastBarTime = 0;
datetime lastSignalTime = 0;
string lastSignal = "NONE";
int lastSignalScore = 0;

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
   double trailingStart;
   double trailingStep;
   datetime openTime;
};

PositionInfo positions[];

struct StrategySettings {
   double trailingStart;
   double trailingStep;
};

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
   if(MaxPositions < 1 || MaxPositions > 10) return(INIT_PARAMETERS_INCORRECT);

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

   ArrayResize(emaFast, 3); ArrayResize(emaSlow, 3);
   ArrayResize(rsi, 3); ArrayResize(bbUpper, 3); ArrayResize(bbLower, 3); ArrayResize(bbMid, 3);
   ArrayResize(macdMain, 3); ArrayResize(macdSignal, 3);

   ArraySetAsSeries(emaFast, true); ArraySetAsSeries(emaSlow, true);
   ArraySetAsSeries(rsi, true); ArraySetAsSeries(bbUpper, true); ArraySetAsSeries(bbLower, true);
   ArraySetAsSeries(bbMid, true); ArraySetAsSeries(macdMain, true); ArraySetAsSeries(macdSignal, true);

   stats.totalSignals = 0; stats.totalTrades = 0; stats.winningTrades = 0; stats.losingTrades = 0; stats.totalProfit = 0; stats.consecutiveLosses = 0;

   SyncPositions();
   return(INIT_SUCCEEDED);
}

//==================== ON DEINIT ====================================//
void OnDeinit(const int reason) {
   IndicatorRelease(emaFastHandle); IndicatorRelease(emaSlowHandle);
   IndicatorRelease(rsiHandle); IndicatorRelease(bbHandle); IndicatorRelease(macdHandle);
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

   if(EnableEquityStop && CheckEquityStop()) { ManagePositions(); return; }
   if(EnableDailyLossStop && CheckDailyLossStop()) { ManagePositions(); return; }

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
            positions[size].strength = "UNKNOWN";
            positions[size].beMovedTo = false;
            positions[size].trailingActive = false;
            positions[size].highest = positions[size].entryPrice;
            positions[size].lowest = positions[size].entryPrice;
            positions[size].trailingStart = 0.0005; // Default fallback
            positions[size].trailingStep = 0.0002;
            positions[size].openTime = (datetime)PositionGetInteger(POSITION_TIME);
         }
      }
   }
}

//==================== SESSION CHECK ================================//
bool IsTradingSession() {
   MqlDateTime dt; TimeToStruct(TimeCurrent() + 7*3600, dt);
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
   // BE Trigger now handled by % TP logic, so we only set Trailing here
   if(strength == "WEAK") { strat.trailingStart=0.0004; strat.trailingStep=0.0001; }
   else if(strength == "MEDIUM") { strat.trailingStart=0.0005; strat.trailingStep=0.0002; }
   else { strat.trailingStart=0.0006; strat.trailingStep=0.0003; }
   return strat;
}

//==================== OPEN POSITIONS ===============================//
void OpenSmartPositions(string signal, string strength, int score) {
   StrategySettings strat = GetStrategySettings(strength);
   double atrM1 = GetATR(PERIOD_M1, ATR_Period);
   if(atrM1 <= 0) atrM1 = SymbolInfoDouble(_Symbol, SYMBOL_POINT) * 50;

   double slDist = atrM1 * ATR_SL_Mult;
   double tpDist = atrM1 * ATR_TP_Mult;

   double lotToTrade = UseDynamicLots ? GetDynamicLot(slDist, RiskPercent, score) : FixedBaseLot;
   if(lotToTrade > MaxLotSize) lotToTrade = MaxLotSize;
   if(lotToTrade < MinLotSize) lotToTrade = MinLotSize;

   int positionsToOpen = (ExecutionMode == MODE_HYBRID_LIMIT) ? 1 : MaxPositions;

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

   if(ExecutionMode != MODE_INSTANT) {
      double pendingEntry = 0;
      double pendingOffset = atrM1 * PendingDistanceATR;
      ENUM_PENDING_TYPE type;

      if(ExecutionMode == MODE_PENDING_LIMIT || ExecutionMode == MODE_HYBRID_LIMIT) {
         if(signal == "BUY") { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_ASK) - pendingOffset; type = PENDING_LIMIT; }
         else { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_BID) + pendingOffset; type = PENDING_LIMIT; }
      }
      else {
         if(signal == "BUY") { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_ASK) + pendingOffset; type = PENDING_STOP; }
         else { pendingEntry = SymbolInfoDouble(_Symbol, SYMBOL_BID) - pendingOffset; type = PENDING_STOP; }
      }

      int pendingCount = (ExecutionMode == MODE_HYBRID_LIMIT) ? 1 : MaxPositions;

      for(int i=0; i<pendingCount; i++) {
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

   double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);
   if(pType == PENDING_LIMIT) {
       if(price < currentPrice) request.type = ORDER_TYPE_BUY_LIMIT;
       else request.type = ORDER_TYPE_SELL_LIMIT;
   } else {
       if(price > currentPrice) request.type = ORDER_TYPE_BUY_STOP;
       else request.type = ORDER_TYPE_SELL_STOP;
   }

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

         if(newSignal == "BUY") {
            if(type == ORDER_TYPE_SELL_LIMIT || type == ORDER_TYPE_SELL_STOP) shouldDelete = true;
         }
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

//==================== MANAGE POSITIONS (UPDATED) ====================//
void ManagePositions() {
   for(int i = ArraySize(positions)-1; i >= 0; i--) {
      if(!PositionSelectByTicket(positions[i].ticket)) continue;

      double currentPrice = (positions[i].side == "BUY") ? SymbolInfoDouble(_Symbol,SYMBOL_BID) : SymbolInfoDouble(_Symbol,SYMBOL_ASK);

      // Track High/Low
      if(positions[i].side == "BUY") { if(currentPrice > positions[i].highest) positions[i].highest = currentPrice; }
      else { if(currentPrice < positions[i].lowest) positions[i].lowest = currentPrice; }

      double profitPct = 0;
      double currentProfitDist = 0;
      double totalTPDist = MathAbs(positions[i].tp - positions[i].entryPrice);

      if(positions[i].side == "BUY") {
         profitPct = (currentPrice - positions[i].entryPrice) / positions[i].entryPrice;
         currentProfitDist = currentPrice - positions[i].entryPrice;
      }
      else {
         profitPct = (positions[i].entryPrice - currentPrice) / positions[i].entryPrice;
         currentProfitDist = positions[i].entryPrice - currentPrice;
      }

      // --- NEW BREAKEVEN LOGIC (30% of TP) ---
      // We check if current profit distance > (Total TP Distance * Percentage)
      if(UseBreakeven && !positions[i].beMovedTo && totalTPDist > 0) {
         if(currentProfitDist >= (totalTPDist * (BE_Trigger_PctTP / 100.0))) {
            double newSL = (positions[i].side == "BUY") ? positions[i].entryPrice + BreakevenOffset : positions[i].entryPrice - BreakevenOffset;
            newSL = NormalizeDouble(newSL, _Digits);

            if(ModifyPosition(positions[i].ticket, newSL, positions[i].tp)) {
               positions[i].sl = newSL;
               positions[i].beMovedTo = true;
               Print("Locked BE for Ticket ", positions[i].ticket, " at 30% TP progress.");
            }
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
   for(int i=PositionsTotal()-1;i>=0;i--) {
      if(PositionGetTicket(i)>0 && PositionGetString(POSITION_SYMBOL)==_Symbol && PositionGetInteger(POSITION_MAGIC)==MagicNumber) count++;
   }
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

   double balance = AccountInfoDouble(ACCOUNT_BALANCE);
   double currentLimit = 0;
   if(balance < BalanceThreshold) currentLimit = FixedLossBelowThreshold;
   else currentLimit = balance * (PctLossAboveThreshold/100.0);

   double dailyProfit = 0;
   datetime start = (datetime)(TimeCurrent() - (TimeCurrent() % 86400));
   if(HistorySelect(start, TimeCurrent())) {
      for(int i=0; i<HistoryDealsTotal(); i++) {
         ulong ticket = HistoryDealGetTicket(i);
         dailyProfit += HistoryDealGetDouble(ticket, DEAL_PROFIT);
         dailyProfit += HistoryDealGetDouble(ticket, DEAL_SWAP);
         dailyProfit += HistoryDealGetDouble(ticket, DEAL_COMMISSION);
      }
   }

   string info = StringFormat(
      "SMART SCALPING BOT v3.22 (Dynamic BE)\nMode: %s\nTime: %02d:%02d UTC+7\nPrice: %.5f\nPositions: %d / %d\nLast Signal: %s\nCurrent Open P/L: $%.2f\nTotal History P/L: $%.2f\n\nDaily P/L: $%.2f\nDaily Limit: -$%.2f\nBE Trigger: %.0f%% of TP",
      EnumToString(ExecutionMode), dt.hour, dt.min, price, openPos, MaxTotalPositions,
      lastSignal, currentProfit, stats.totalProfit, dailyProfit, currentLimit, BE_Trigger_PctTP
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

double GetDynamicLot(double slDistancePoints, double riskPct, int score) {
   if(!UseDynamicLots) return FixedBaseLot;
   double balance = AccountInfoDouble(ACCOUNT_BALANCE);
   double riskMoney = (balance * (riskPct / 100.0)) / (double)MaxPositions;
   double mult = 1.0;
   if(score >= 9) mult = 2.0;
   else if(score >= 7) mult = 1.5;
   else mult = 1.0;
   riskMoney *= mult;
   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
   double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
   if(tickValue <= 0 || tickSize <= 0) return FixedBaseLot;
   double lossPerLot = (slDistancePoints / tickSize) * tickValue;
   if(lossPerLot <= 0) return FixedBaseLot;
   double rawLot = riskMoney / lossPerLot;
   double step = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
   rawLot = MathFloor(rawLot / step) * step;
   if(rawLot < MinLotSize) rawLot = MinLotSize;
   if(rawLot > MaxLotSize) rawLot = MaxLotSize;
   return rawLot;
}

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
         profit += HistoryDealGetDouble(ticket, DEAL_SWAP);
         profit += HistoryDealGetDouble(ticket, DEAL_COMMISSION);
      }
   }
   double balance = AccountInfoDouble(ACCOUNT_BALANCE);
   double currentLimit = 0.0;
   if(balance < BalanceThreshold) {
      currentLimit = FixedLossBelowThreshold;
   } else {
      currentLimit = balance * (PctLossAboveThreshold / 100.0);
   }
   return (profit <= -currentLimit);
}
//+------------------------------------------------------------------+