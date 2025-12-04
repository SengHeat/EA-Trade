//+------------------------------------------------------------------+
//|                          AggressiveSwingBot_v1.mq5               |
//|           High Risk Swing Trader - 50% Balance Per Signal        |
//+------------------------------------------------------------------+
#property copyright "Aggressive Swing Bot v1.0"
#property version   "1.00"
#property strict

//==================== INPUT PARAMETERS ==============================//
input group "=== AGGRESSIVE RISK SETTINGS ===";
input double   RiskPercentPerSignal = 50.0;    // Risk % Per Signal (50% = EXTREME RISK!)
input int      MaxPositionsPerSignal = 1;      // Positions per signal
input double   MaxLotSize = 100.0;             // Maximum lot size cap
input double   MinLotSize = 0.01;              // Minimum lot size

input group "=== SWING TRADING SETTINGS ===";
input ENUM_TIMEFRAMES SwingTimeframe = PERIOD_H4;  // Primary Swing Timeframe
input int      EMA_Fast = 20;                  // Fast EMA (Swing)
input int      EMA_Slow = 50;                  // Slow EMA (Swing)
input int      EMA_Trend = 200;                // Trend Filter EMA
input int      RSI_Period = 14;                // RSI Period
input double   RSI_Oversold = 30;              // RSI Oversold
input double   RSI_Overbought = 70;            // RSI Overbought
input int      ADX_Period = 14;                // ADX Period
input double   ADX_MinStrength = 20;           // Minimum ADX for trend

input group "=== STOP LOSS & TAKE PROFIT ===";
input double   ATR_Period = 14;                // ATR Period
input double   SL_ATR_Multiplier = 2.0;        // Stop Loss = ATR x This
input double   TP_ATR_Multiplier = 4.0;        // Take Profit = ATR x This (1:2 Risk/Reward)

input group "=== TRADING HOURS (18 HOURS/DAY) ===";
input int      TradingStartHour = 0;           // Start Hour (UTC+7)
input int      TradingEndHour = 18;            // End Hour (UTC+7) - 18 hours trading

input group "=== MANAGEMENT ===";
input bool     UseTrailingStop = true;         // Enable Trailing Stop
input double   TrailingStopATR = 1.5;          // Trailing Stop ATR Multiplier
input bool     UseBreakeven = true;            // Move to Breakeven
input double   BreakevenTriggerATR = 1.0;      // Breakeven Trigger (ATR x)
input int      MagicNumber = 888999;           // Magic Number
input bool     OneSignalAtATime = true;        // Only 1 signal active at a time

//==================== GLOBALS ======================================//
int emaFastHandle, emaSlowHandle, emaTrendHandle, rsiHandle, adxHandle, atrHandle;
double emaFast[], emaSlow[], emaTrend[], rsi[], adxMain[], atrBuffer[];
datetime lastBarTime = 0;
datetime lastSignalTime = 0;
string currentSignal = "NONE";
bool signalActive = false;

struct PositionData {
   ulong ticket;
   string type;
   double entryPrice;
   double sl;
   double tp;
   double lotSize;
   bool beActive;
   double highestPrice;
   double lowestPrice;
};
PositionData activePositions[];

//==================== ON INIT ======================================//
int OnInit() {
   Print("========================================");
   Print("AGGRESSIVE SWING TRADING BOT - 50% RISK");
   Print("⚠️ WARNING: EXTREME RISK SETTINGS! ⚠️");
   Print("Risk Per Signal: ", RiskPercentPerSignal, "%");
   Print("Timeframe: ", EnumToString(SwingTimeframe));
   Print("Trading Hours: ", TradingStartHour, ":00 to ", TradingEndHour, ":00 UTC+7");
   Print("========================================");

   // Initialize indicators on swing timeframe
   emaFastHandle = iMA(_Symbol, SwingTimeframe, EMA_Fast, 0, MODE_EMA, PRICE_CLOSE);
   emaSlowHandle = iMA(_Symbol, SwingTimeframe, EMA_Slow, 0, MODE_EMA, PRICE_CLOSE);
   emaTrendHandle = iMA(_Symbol, SwingTimeframe, EMA_Trend, 0, MODE_EMA, PRICE_CLOSE);
   rsiHandle = iRSI(_Symbol, SwingTimeframe, RSI_Period, PRICE_CLOSE);
   adxHandle = iADX(_Symbol, SwingTimeframe, ADX_Period);
   atrHandle = iATR(_Symbol, SwingTimeframe, (int)ATR_Period);

   if(emaFastHandle == INVALID_HANDLE || emaSlowHandle == INVALID_HANDLE || 
      emaTrendHandle == INVALID_HANDLE || rsiHandle == INVALID_HANDLE ||
      adxHandle == INVALID_HANDLE || atrHandle == INVALID_HANDLE) {
      Print("ERROR: Failed to create indicators!");
      return INIT_FAILED;
   }

   ArrayResize(emaFast, 3); ArrayResize(emaSlow, 3); ArrayResize(emaTrend, 3);
   ArrayResize(rsi, 3); ArrayResize(adxMain, 3); ArrayResize(atrBuffer, 1);
   
   ArraySetAsSeries(emaFast, true); ArraySetAsSeries(emaSlow, true); 
   ArraySetAsSeries(emaTrend, true); ArraySetAsSeries(rsi, true);
   ArraySetAsSeries(adxMain, true);

   SyncPositions();
   
   return INIT_SUCCEEDED;
}

//==================== ON DEINIT ====================================//
void OnDeinit(const int reason) {
   IndicatorRelease(emaFastHandle);
   IndicatorRelease(emaSlowHandle);
   IndicatorRelease(emaTrendHandle);
   IndicatorRelease(rsiHandle);
   IndicatorRelease(adxHandle);
   IndicatorRelease(atrHandle);
}

//==================== ON TICK ======================================//
void OnTick() {
   // Check for new bar on swing timeframe
   datetime currentBarTime = iTime(_Symbol, SwingTimeframe, 0);
   bool isNewBar = (currentBarTime != lastBarTime);
   
   if(!isNewBar) {
      ManageOpenPositions();
      return;
   }
   
   lastBarTime = currentBarTime;
   
   // Update indicators
   if(!UpdateIndicators()) return;
   
   // Sync positions
   SyncPositions();
   
   // Check trading hours
   if(!IsTradingHours()) {
      UpdateDisplay();
      return;
   }
   
   // Check if we can trade
   bool canTrade = true;
   if(OneSignalAtATime && ArraySize(activePositions) > 0) {
      canTrade = false; // Wait for current position to close
   }
   
   if(canTrade) {
      string signal = AnalyzeSwingSignal();
      
      if(signal != "HOLD" && signal != currentSignal) {
         Print("🎯 NEW SWING SIGNAL: ", signal);
         ExecuteAggressiveTrade(signal);
         currentSignal = signal;
         lastSignalTime = TimeCurrent();
      }
   }
   
   ManageOpenPositions();
   UpdateDisplay();
}

//==================== UPDATE INDICATORS ============================//
bool UpdateIndicators() {
   if(CopyBuffer(emaFastHandle, 0, 0, 3, emaFast) <= 0) return false;
   if(CopyBuffer(emaSlowHandle, 0, 0, 3, emaSlow) <= 0) return false;
   if(CopyBuffer(emaTrendHandle, 0, 0, 3, emaTrend) <= 0) return false;
   if(CopyBuffer(rsiHandle, 0, 0, 3, rsi) <= 0) return false;
   if(CopyBuffer(adxHandle, 0, 0, 3, adxMain) <= 0) return false;
   if(CopyBuffer(atrHandle, 0, 0, 1, atrBuffer) <= 0) return false;
   return true;
}

//==================== TRADING HOURS CHECK ==========================//
bool IsTradingHours() {
   MqlDateTime dt;
   TimeToStruct(TimeCurrent() + 7*3600, dt); // UTC+7 Cambodia time
   int hour = dt.hour;
   
   // Trade 18 hours: from TradingStartHour to TradingEndHour
   return (hour >= TradingStartHour && hour < TradingEndHour);
}

//==================== ANALYZE SWING SIGNAL =========================//
string AnalyzeSwingSignal() {
   double currentPrice = iClose(_Symbol, SwingTimeframe, 0);
   
   // 1. TREND FILTER - Must be above/below 200 EMA
   bool uptrend = (currentPrice > emaTrend[0]);
   bool downtrend = (currentPrice < emaTrend[0]);
   
   // 2. EMA CROSSOVER
   bool emaBullish = (emaFast[0] > emaSlow[0]);
   bool emaBearish = (emaFast[0] < emaSlow[0]);
   bool emaCrossUp = (emaFast[0] > emaSlow[0] && emaFast[1] <= emaSlow[1]);
   bool emaCrossDown = (emaFast[0] < emaSlow[0] && emaFast[1] >= emaSlow[1]);
   
   // 3. RSI CONFIRMATION
   bool rsiOversold = (rsi[0] < RSI_Oversold);
   bool rsiOverbought = (rsi[0] > RSI_Overbought);
   bool rsiRising = (rsi[0] > rsi[1]);
   bool rsiFalling = (rsi[0] < rsi[1]);
   
   // 4. ADX - Trend Strength
   bool strongTrend = (adxMain[0] > ADX_MinStrength);
   
   // BUY CONDITIONS
   if(uptrend && emaBullish && strongTrend) {
      if(emaCrossUp || (rsiOversold && rsiRising)) {
         return "BUY";
      }
   }
   
   // SELL CONDITIONS  
   if(downtrend && emaBearish && strongTrend) {
      if(emaCrossDown || (rsiOverbought && rsiFalling)) {
         return "SELL";
      }
   }
   
   return "HOLD";
}

//==================== EXECUTE AGGRESSIVE TRADE =====================//
void ExecuteAggressiveTrade(string signal) {
   double atr = atrBuffer[0];
   if(atr <= 0) {
      Print("Invalid ATR, skipping trade");
      return;
   }
   
   // Calculate SL and TP distances
   double slDistance = atr * SL_ATR_Multiplier;
   double tpDistance = atr * TP_ATR_Multiplier;
   
   // Calculate lot size for 50% risk
   double lotSize = CalculateAggressiveLotSize(slDistance);
   
   // Get entry price
   double entryPrice = (signal == "BUY") ? 
      SymbolInfoDouble(_Symbol, SYMBOL_ASK) : 
      SymbolInfoDouble(_Symbol, SYMBOL_BID);
   
   // Calculate SL and TP
   double sl, tp;
   if(signal == "BUY") {
      sl = entryPrice - slDistance;
      tp = entryPrice + tpDistance;
   } else {
      sl = entryPrice + slDistance;
      tp = entryPrice - tpDistance;
   }
   
   // Validate and normalize
   sl = NormalizeDouble(sl, _Digits);
   tp = NormalizeDouble(tp, _Digits);
   
   // Open position
   string comment = StringFormat("SWING_%s_50PCT", signal);
   ulong ticket = OpenMarketOrder(signal, lotSize, sl, tp, comment);
   
   if(ticket > 0) {
      Print("✅ AGGRESSIVE SWING TRADE OPENED");
      Print("   Ticket: ", ticket);
      Print("   Signal: ", signal);
      Print("   Lot Size: ", lotSize);
      Print("   Entry: ", entryPrice);
      Print("   SL: ", sl, " (", slDistance/_Point, " points)");
      Print("   TP: ", tp, " (", tpDistance/_Point, " points)");
      Print("   Risk/Reward: 1:", TP_ATR_Multiplier/SL_ATR_Multiplier);
   }
}

//==================== CALCULATE AGGRESSIVE LOT SIZE ================//
double CalculateAggressiveLotSize(double slDistancePrice) {
   double balance = AccountInfoDouble(ACCOUNT_BALANCE);
   double riskMoney = balance * (RiskPercentPerSignal / 100.0);
   
   // Calculate lot size based on risk
   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
   double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
   
   if(tickValue <= 0 || tickSize <= 0) {
      Print("Invalid tick values");
      return MinLotSize;
   }
   
   double slDistancePoints = slDistancePrice / tickSize;
   double lossPerLot = slDistancePoints * tickValue;
   
   if(lossPerLot <= 0) return MinLotSize;
   
   double lotSize = riskMoney / lossPerLot;
   
   // Apply constraints
   double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
   double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
   double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
   
   lotSize = MathFloor(lotSize / lotStep) * lotStep;
   lotSize = MathMax(lotSize, minLot);
   lotSize = MathMin(lotSize, maxLot);
   lotSize = MathMin(lotSize, MaxLotSize);
   
   Print("💰 AGGRESSIVE LOT CALCULATION:");
   Print("   Balance: $", balance);
   Print("   Risk Money (50%): $", riskMoney);
   Print("   SL Distance: ", slDistancePrice);
   Print("   Calculated Lot: ", lotSize);
   
   return lotSize;
}

//==================== OPEN MARKET ORDER ============================//
ulong OpenMarketOrder(string side, double lot, double sl, double tp, string comment) {
   MqlTradeRequest request;
   MqlTradeResult result;
   ZeroMemory(request);
   ZeroMemory(result);
   
   request.action = TRADE_ACTION_DEAL;
   request.symbol = _Symbol;
   request.volume = lot;
   request.type = (side == "BUY") ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
   request.price = (side == "BUY") ? 
      SymbolInfoDouble(_Symbol, SYMBOL_ASK) : 
      SymbolInfoDouble(_Symbol, SYMBOL_BID);
   request.sl = sl;
   request.tp = tp;
   request.deviation = 50;
   request.magic = MagicNumber;
   request.comment = comment;
   request.type_filling = ORDER_FILLING_IOC;
   
   if(!OrderSend(request, result)) {
      Print("Order failed: ", GetLastError());
      return 0;
   }
   
   return result.order;
}

//==================== SYNC POSITIONS ===============================//
void SyncPositions() {
   // Remove closed positions
   for(int i = ArraySize(activePositions) - 1; i >= 0; i--) {
      if(!PositionSelectByTicket(activePositions[i].ticket)) {
         ArrayRemove(activePositions, i);
         if(ArraySize(activePositions) == 0) {
            currentSignal = "NONE"; // Reset signal when all positions closed
         }
      }
   }
   
   // Add new positions
   for(int i = PositionsTotal() - 1; i >= 0; i--) {
      ulong ticket = PositionGetTicket(i);
      if(ticket > 0 && PositionGetString(POSITION_SYMBOL) == _Symbol && 
         PositionGetInteger(POSITION_MAGIC) == MagicNumber) {
         
         bool found = false;
         for(int j = 0; j < ArraySize(activePositions); j++) {
            if(activePositions[j].ticket == ticket) {
               found = true;
               break;
            }
         }
         
         if(!found) {
            int size = ArraySize(activePositions);
            ArrayResize(activePositions, size + 1);
            activePositions[size].ticket = ticket;
            activePositions[size].type = (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) ? "BUY" : "SELL";
            activePositions[size].entryPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            activePositions[size].sl = PositionGetDouble(POSITION_SL);
            activePositions[size].tp = PositionGetDouble(POSITION_TP);
            activePositions[size].lotSize = PositionGetDouble(POSITION_VOLUME);
            activePositions[size].beActive = false;
            activePositions[size].highestPrice = activePositions[size].entryPrice;
            activePositions[size].lowestPrice = activePositions[size].entryPrice;
         }
      }
   }
}

//==================== MANAGE OPEN POSITIONS ========================//
void ManageOpenPositions() {
   double atr = atrBuffer[0];
   
   for(int i = 0; i < ArraySize(activePositions); i++) {
      if(!PositionSelectByTicket(activePositions[i].ticket)) continue;
      
      double currentPrice = (activePositions[i].type == "BUY") ?
         SymbolInfoDouble(_Symbol, SYMBOL_BID) :
         SymbolInfoDouble(_Symbol, SYMBOL_ASK);
      
      // Track highest/lowest
      if(activePositions[i].type == "BUY") {
         if(currentPrice > activePositions[i].highestPrice)
            activePositions[i].highestPrice = currentPrice;
      } else {
         if(currentPrice < activePositions[i].lowestPrice)
            activePositions[i].lowestPrice = currentPrice;
      }
      
      // BREAKEVEN
      if(UseBreakeven && !activePositions[i].beActive) {
         double beTrigger = atr * BreakevenTriggerATR;
         bool shouldMoveBE = false;
         
         if(activePositions[i].type == "BUY") {
            if(currentPrice >= activePositions[i].entryPrice + beTrigger)
               shouldMoveBE = true;
         } else {
            if(currentPrice <= activePositions[i].entryPrice - beTrigger)
               shouldMoveBE = true;
         }
         
         if(shouldMoveBE) {
            double newSL = activePositions[i].entryPrice;
            if(ModifyPosition(activePositions[i].ticket, newSL, activePositions[i].tp)) {
               activePositions[i].sl = newSL;
               activePositions[i].beActive = true;
               Print("✓ Breakeven activated for #", activePositions[i].ticket);
            }
         }
      }
      
      // TRAILING STOP
      if(UseTrailingStop && activePositions[i].beActive) {
         double trailDistance = atr * TrailingStopATR;
         double newSL;
         
         if(activePositions[i].type == "BUY") {
            newSL = activePositions[i].highestPrice - trailDistance;
            if(newSL > activePositions[i].sl) {
               if(ModifyPosition(activePositions[i].ticket, newSL, activePositions[i].tp)) {
                  activePositions[i].sl = newSL;
               }
            }
         } else {
            newSL = activePositions[i].lowestPrice + trailDistance;
            if(newSL < activePositions[i].sl) {
               if(ModifyPosition(activePositions[i].ticket, newSL, activePositions[i].tp)) {
                  activePositions[i].sl = newSL;
               }
            }
         }
      }
   }
}

//==================== MODIFY POSITION ==============================//
bool ModifyPosition(ulong ticket, double sl, double tp) {
   MqlTradeRequest request;
   MqlTradeResult result;
   ZeroMemory(request);
   ZeroMemory(result);
   
   request.action = TRADE_ACTION_SLTP;
   request.symbol = _Symbol;
   request.position = ticket;
   request.sl = NormalizeDouble(sl, _Digits);
   request.tp = NormalizeDouble(tp, _Digits);
   request.magic = MagicNumber;
   
   return OrderSend(request, result);
}

//==================== UPDATE DISPLAY ===============================//
void UpdateDisplay() {
   MqlDateTime dt;
   TimeToStruct(TimeCurrent() + 7*3600, dt);
   
   double currentProfit = 0;
   for(int i = 0; i < ArraySize(activePositions); i++) {
      if(PositionSelectByTicket(activePositions[i].ticket)) {
         currentProfit += PositionGetDouble(POSITION_PROFIT);
      }
   }
   
   double balance = AccountInfoDouble(ACCOUNT_BALANCE);
   double equity = AccountInfoDouble(ACCOUNT_EQUITY);
   
   string info = StringFormat(
      "═══════════════════════════════════════\n" +
      "⚡ AGGRESSIVE SWING TRADING BOT v1.0\n" +
      "═══════════════════════════════════════\n" +
      "⚠️  RISK PER SIGNAL: %.0f%% BALANCE\n" +
      "───────────────────────────────────────\n" +
      "Time: %02d:%02d:%02d UTC+7\n" +
      "Timeframe: %s\n" +
      "Trading Hours: %02d:00 - %02d:00\n" +
      "───────────────────────────────────────\n" +
      "Balance: $%.2f\n" +
      "Equity: $%.2f\n" +
      "Current P/L: $%.2f\n" +
      "───────────────────────────────────────\n" +
      "Active Positions: %d\n" +
      "Current Signal: %s\n" +
      "ADX Strength: %.1f\n" +
      "ATR: %.5f\n" +
      "═══════════════════════════════════════",
      RiskPercentPerSignal,
      dt.hour, dt.min, dt.sec,
      EnumToString(SwingTimeframe),
      TradingStartHour, TradingEndHour,
      balance,
      equity,
      currentProfit,
      ArraySize(activePositions),
      currentSignal,
      adxMain[0],
      atrBuffer[0]
   );
   
   Comment(info);
}

//==================== UTILITY: ARRAY REMOVE ========================//
template<typename T>
void ArrayRemove(T &arr[], int index) {
   int size = ArraySize(arr);
   if(index < 0 || index >= size) return;
   
   for(int i = index; i < size - 1; i++) {
      arr[i] = arr[i + 1];
   }
   ArrayResize(arr, size - 1);
}
//+------------------------------------------------------------------+