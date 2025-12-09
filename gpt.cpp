//+------------------------------------------------------------------+
//|                                              SmartScalpingBot_v4 |
//|                                     Copyright 2025, MetaQuotes   |
//+------------------------------------------------------------------+
#property copyright "Copyright 2025, MetaQuotes"
#property link      "https://www.mql5.com"
#property version   "4.00"
#property strict

//==================== STRUCTURES ====================================//
struct PriceActionData {
    bool isEngulfing;
    bool isDoji;
    bool isInsideBar;
    int direction;
    int strength;
};

struct OrderFlowData {
    double buyVolume;
    double sellVolume;
    double volumeImbalance;
    double priceVelocity;
    int momentum;
};

struct PositionInfo {
    ulong ticket;
    string side;
    double entryPrice;
    double lotSize;
    double sl;
    double tp;
    int level;
    string strength;
    int entryScore;
    bool beMovedTo;
    bool trailingActive;
    double highest;
    double lowest;
    double beThreshold;
    double trailingStart;
    double trailingStep;
    datetime openTime;
};

struct TradingStats {
    int totalTrades;
    int todayTrades;
    int winningTrades;
    int losingTrades;
    int consecutiveLosses;
    double totalProfit;
    double todayProfit;
    double maxDrawdown;
    double peakEquity;
};

//==================== INPUTS ========================================//
input ENUM_TIMEFRAMES TrendTF1 = PERIOD_H1;    // Trend Timeframe 1
input ENUM_TIMEFRAMES TrendTF2 = PERIOD_H4;    // Trend Timeframe 2
input int TrendEMA1 = 50;                      // EMA Period 1
input int TrendEMA2 = 200;                     // EMA Period 2
input double ATR_SL_Mult = 2.0;                // ATR Multiplier for SL
input double ATR_TP_Mult = 1.50;                // ATR Multiplier for TP
input bool UseDynamicLots = true;              // Use Dynamic Lot Sizing
input double RiskPercent = 1.0;                // Risk Percent per Trade
input double FixedBaseLot = 0.01;              // Fixed Lot Size
input double MaxLotSize = 5.0;                 // Max Allowed Lot
input int MaxPositions = 1;                    // Max Positions per Signal
input int MaxTotalPositions = 10;              // Max Total Open Positions
input int Slippage = 3;                        // Max Slippage
input int MagicNumber = 123456;                // Magic Number
input bool UseBreakeven = true;                // Use Breakeven
input double BreakevenOffset = 0.0001;         // Breakeven Offset
input bool UseTrailing = true;                 // Use Trailing Stop
input bool UseAsianSession = false;            // Trade Asian Session
input bool UseLondonSession = true;            // Trade London Session
input bool UseNYSession = true;                // Trade NY Session
input bool EnableEquityStop = true;            // Enable Max Equity Drawdown Stop
input double MaxEquityDrawdown = 0.10;         // Max Drawdown (0.10 = 10%)
input bool EnableDailyLossStop = true;         // Enable Daily Loss Limit
input double DailyLossLimit = 100.0;           // Daily Loss Limit ($)
input bool AllowMultipleSignals = true;        // Allow Multiple Signals
input bool IgnoreMaxPositionLimit = false;  // Add this to inputs section



//==================== GLOBAL VARIABLES ==============================//
double rsi[];
double atr[];
double emaFast[];
double emaSlow[];
PositionInfo positions[];
TradingStats stats;
string lastSignal = "NONE";
int lastSignalScore = 0;

// Indicators Handles
int hRSI, hATR, hEMAFast, hEMASlow;

//==================== INITIALIZATION ================================//
int OnInit() {
    ArrayResize(positions, 0);
    ZeroMemory(stats);

    // Initialize Indicators
    hRSI = iRSI(_Symbol, PERIOD_M1, 14, PRICE_CLOSE);
    hATR = iATR(_Symbol, PERIOD_M1, 14);
    hEMAFast = iMA(_Symbol, PERIOD_M1, 9, 0, MODE_EMA, PRICE_CLOSE);
    hEMASlow = iMA(_Symbol, PERIOD_M1, 21, 0, MODE_EMA, PRICE_CLOSE);

    if(hRSI == INVALID_HANDLE || hATR == INVALID_HANDLE) return INIT_FAILED;

    return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason) {
    IndicatorRelease(hRSI);
    IndicatorRelease(hATR);
    IndicatorRelease(hEMAFast);
    IndicatorRelease(hEMASlow);
}

void OnTick() {
    // 1. Update Indicator Buffers
    CopyBuffer(hRSI, 0, 0, 5, rsi);
    CopyBuffer(hATR, 0, 0, 5, atr);
    CopyBuffer(hEMAFast, 0, 0, 5, emaFast);
    CopyBuffer(hEMASlow, 0, 0, 5, emaSlow);
    ArraySetAsSeries(rsi, true);
    ArraySetAsSeries(atr, true);
    ArraySetAsSeries(emaFast, true);
    ArraySetAsSeries(emaSlow, true);

    // 2. Manage Existing Positions & Display
    ManagePositions();
    SyncPositions();
    UpdateStats();
    UpdateDisplay();

    // 3. Filters (Time, News, Drawdown)
    if(!IsTradingSession() || IsNewsTime() || CheckEquityStop() || CheckDailyLossStop()) return;

    //================ ENTRY LOGIC & DEBUG PRINTS ==================//

    // We only want to check for entry once per bar (on the new candle open)
    datetime currentBarTime = iTime(_Symbol, Period(), 0);

    // --- A. GATHER DATA ---
    PriceActionData pa = AnalyzePriceAction();
    OrderFlowData flow = AnalyzeOrderFlow();
    int trend = GetHigherTFTrend();
    bool rsiDiv = CheckRSIDivergence();

    int score = 0;
    string signal = "NONE";

    // --- B. CALCULATE SIGNAL & SCORE ---

    // Check BUY Signal
    if(pa.direction == 1 || (pa.isEngulfing && pa.direction == 1)) {
        signal = "BUY";
        score += 5; // Base score for Price Action
        if(pa.strength > 70) score += 3; // Strong candle
        if(trend == 1) score += 4; // Trend Alignment
        if(flow.momentum == 1) score += 3; // Order Flow
        if(rsiDiv) score += 3; // Divergence
        if(rsi[1] < 30) score += 2; // RSI Oversold
    }
    // Check SELL Signal
    else if(pa.direction == -1 || (pa.isEngulfing && pa.direction == -1)) {
        signal = "SELL";
        score += 5;
        if(pa.strength > 70) score += 3;
        if(trend == -1) score += 4;
        if(flow.momentum == -1) score += 3;
        if(rsiDiv) score += 3;
        if(rsi[1] > 70) score += 2; // RSI Overbought
    }

    // --- C. PRINT DEBUG INFO (What you asked for) ---
    if(signal != "NONE") {
        Print("╔════════════ ENTRY SIGNAL DETECTED ════════════");
        Print("║ Signal: ", signal);
        Print("║ ----------------------------------------------");
        Print("║ 1. Price Action: ", (pa.isEngulfing ? "Engulfing" : "Standard"), " | Strength: ", pa.strength);
        Print("║ 2. Trend Filter: ", (trend == 1 ? "BULLISH" : (trend == -1 ? "BEARISH" : "FLAT")));
        Print("║ 3. Order Flow:   ", (flow.momentum == 1 ? "BUY Pressure" : (flow.momentum == -1 ? "SELL Pressure" : "Neutral")));
        Print("║ 4. RSI Div:      ", (rsiDiv ? "YES" : "NO"));
        Print("║ ----------------------------------------------");
        Print("║ FINAL SCORE: ", score, " (Required: 8)");

        string strengthLabel = GetSignalStrength(score);

        if(score >= 8) {
            Print("║ ✅ DECISION: EXECUTE TRADE (", strengthLabel, ")");
            Print("╚═══════════════════════════════════════════════");

            // Execute Trade
            OpenSmartPositions(signal, strengthLabel, score);

            // Update Global Variables for Display
            lastSignal = signal;
            lastSignalScore = score;
        } else {
            Print("║ ❌ DECISION: IGNORE (Score too low)");
            Print("╚═══════════════════════════════════════════════");
        }
    }
}

//==================== PRICE ACTION ANALYSIS =========================//
PriceActionData AnalyzePriceAction() {
    PriceActionData pa;
    ZeroMemory(pa);

    double open0 = iOpen(_Symbol, PERIOD_M1, 0);
    double close0 = iClose(_Symbol, PERIOD_M1, 0);
    double high0 = iHigh(_Symbol, PERIOD_M1, 0);
    double low0 = iLow(_Symbol, PERIOD_M1, 0);

    double open1 = iOpen(_Symbol, PERIOD_M1, 1);
    double close1 = iClose(_Symbol, PERIOD_M1, 1);
    double high1 = iHigh(_Symbol, PERIOD_M1, 1);
    double low1 = iLow(_Symbol, PERIOD_M1, 1);

    double body0 = MathAbs(close0 - open0);
    double range0 = high0 - low0;
    double body1 = MathAbs(close1 - open1);

    // Reconstructed Logic to match your snippet:
    // Detecting Hammer/Shooting Star (implied by the closing brace in your snippet)
    if(range0 > 0 && body0 > 0) {
        if(close0 > open0 && (high0 - close0) > (body0 * 2)) {
             pa.direction = -1; // Shooting Star
             pa.strength = 80;
        }
    }
    // This connects to the start of your provided code snippet:
    else if(close0 < open0 && close1 > open1) {
        if(body0 > body1 * 1.2 && close0 < open1 && open0 > close1) {
            pa.isEngulfing = true;
            pa.direction = -1;
            pa.strength = 80;
        }
    }

    // Doji Detection
    if(range0 > 0 && body0 / range0 < 0.1) {
        pa.isDoji = true;
        pa.strength = 50;
    }

    // Inside Bar
    if(high0 < high1 && low0 > low1) {
        pa.isInsideBar = true;
        pa.strength = 60;
    }

    return pa;
}

//==================== ORDER FLOW ANALYSIS ===========================//
OrderFlowData AnalyzeOrderFlow() {
    OrderFlowData flow;
    ZeroMemory(flow);

    double buyVol = 0, sellVol = 0;

    for(int i = 0; i < 5; i++) {
        double open = iOpen(_Symbol, PERIOD_M1, i);
        double close = iClose(_Symbol, PERIOD_M1, i);
        double vol = (double)iVolume(_Symbol, PERIOD_M1, i);

        if(close > open) buyVol += vol;
        else sellVol += vol;
    }

    flow.buyVolume = buyVol;
    flow.sellVolume = sellVol;
    flow.volumeImbalance = (sellVol > 0) ? (buyVol / sellVol) : 1.0;

    // Price velocity
    double price0 = iClose(_Symbol, PERIOD_M1, 0);
    double price5 = iClose(_Symbol, PERIOD_M1, 5);
    flow.priceVelocity = (price0 - price5) / price5;

    // Momentum
    if(flow.volumeImbalance > 1.2) flow.momentum = 1;
    else if(flow.volumeImbalance < 0.8) flow.momentum = -1;
    else flow.momentum = 0;

    return flow;
}

//==================== RSI DIVERGENCE CHECK ==========================//
bool CheckRSIDivergence() {
    if(ArraySize(rsi) < 5) return false;

    double price0 = iClose(_Symbol, PERIOD_M1, 0);
    double price4 = iClose(_Symbol, PERIOD_M1, 4);

    // Bullish divergence: price lower, RSI higher
    if(price0 < price4 && rsi[0] > rsi[4]) return true;

    // Bearish divergence: price higher, RSI lower
    if(price0 > price4 && rsi[0] < rsi[4]) return true;

    return false;
}

//==================== HIGHER TIMEFRAME TREND ========================//
double GetEMA(ENUM_TIMEFRAMES tf, int period) {
    int h = iMA(_Symbol, tf, period, 0, MODE_EMA, PRICE_CLOSE);
    if(h == INVALID_HANDLE) return 0;

    double buf[1];
    if(CopyBuffer(h, 0, 0, 1, buf) <= 0) {
        IndicatorRelease(h);
        return 0;
    }
    IndicatorRelease(h);
    return buf[0];
}

int GetHigherTFTrend() {
    double ema1 = GetEMA(TrendTF1, TrendEMA1);
    double ema2 = GetEMA(TrendTF2, TrendEMA2);

    if(ema1 == 0 || ema2 == 0) return 0;

    double priceTF1 = iClose(_Symbol, TrendTF1, 0);
    double priceTF2 = iClose(_Symbol, TrendTF2, 0);

    int up = 0, down = 0;

    if(priceTF1 > ema1) up++; else down++;
    if(priceTF2 > ema2) up++; else down++;

    if(up > down) return 1;
    if(down > up) return -1;
    return 0;
}

//==================== SIGNAL STRENGTH ===============================//
string GetSignalStrength(int score) {
    if(score >= 15) return "VERY_STRONG";
    if(score >= 11) return "STRONG";
    if(score >= 8) return "MEDIUM";
    return "WEAK";
}

//==================== OPEN SMART POSITIONS ==========================//
double GetLotMultiplier(string strength) {
    if(strength == "WEAK") return 1.0;
    if(strength == "MEDIUM") return 1.5;
    if(strength == "STRONG") return 2.0;
    return 2.5; // VERY_STRONG
}

double GetBEThreshold(string strength) {
    if(strength == "WEAK") return 0.0002;
    if(strength == "MEDIUM") return 0.0004;
    if(strength == "STRONG") return 0.0006;
    return 0.0008;
}

double GetTrailingStart(string strength) {
    if(strength == "WEAK") return 0.0004;
    if(strength == "MEDIUM") return 0.0006;
    if(strength == "STRONG") return 0.0008;
    return 0.0010;
}

double GetTrailingStep(string strength) {
    if(strength == "WEAK") return 0.0001;
    if(strength == "MEDIUM") return 0.0002;
    if(strength == "STRONG") return 0.0003;
    return 0.0004;
}

double GetDynamicLot(double slDistance, double riskPct, string strength) {
    if(!UseDynamicLots) return FixedBaseLot;

    double equity = AccountInfoDouble(ACCOUNT_EQUITY);
    double riskMoney = (equity * (riskPct / 100.0)) / (double)MaxPositions;

    double strengthMult = GetLotMultiplier(strength);
    riskMoney *= strengthMult;

    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    if(tickValue <= 0 || tickSize <= 0) return FixedBaseLot;

    double lossPerLot = (slDistance / tickSize) * tickValue;
    if(lossPerLot <= 0) return FixedBaseLot;

    double lot = riskMoney / lossPerLot;

    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    double step = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);

    lot = MathFloor(lot / step) * step;
    if(lot < minLot) lot = minLot;
    if(lot > maxLot) lot = maxLot;
    if(lot > MaxLotSize) lot = MaxLotSize;

    return lot;
}

ulong OpenOrder(string side, double lot, double sl, double tp, string comment) {
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
    request.deviation = Slippage;
    request.magic = MagicNumber;
    request.comment = comment;
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
        return 0;
    }

    return result.order;
}

bool ValidateStops(double entry, double &sl, double &tp, string side) {
    int stopLevel = (int)SymbolInfoInteger(_Symbol, SYMBOL_TRADE_STOPS_LEVEL);
    double minDist = MathMax(stopLevel * _Point, 10 * _Point);

    if(side == "BUY") {
        if(entry - sl < minDist) sl = entry - minDist;
        if(tp - entry < minDist) tp = entry + minDist;
        if(sl >= entry || tp <= entry) return false;
    } else {
        if(sl - entry < minDist) sl = entry + minDist;
        if(entry - tp < minDist) tp = entry - minDist;
        if(tp >= entry || sl <= entry) return false;
    }

    sl = NormalizeDouble(sl, _Digits);
    tp = NormalizeDouble(tp, _Digits);
    return true;
}

bool CanOpenMorePositions() {
    if(IgnoreMaxPositionLimit) return true;

    int currentOpen = CountOpenPositions();
    return (currentOpen < MaxTotalPositions);
}

void OpenSmartPositions(string signal, string strength, int score) {
     // Remove this check to allow unlimited positions:
     int currentOpen = CountOpenPositions();
     if(currentOpen >= MaxTotalPositions) {
         Print("Max positions reached: ", currentOpen);
         return;
     }
    double price = (signal == "BUY") ?
        SymbolInfoDouble(_Symbol, SYMBOL_ASK) :
        SymbolInfoDouble(_Symbol, SYMBOL_BID);

    // Calculate ATR-based stops
    double atrValue = atr[0];
    if(atrValue <= 0) atrValue = SymbolInfoDouble(_Symbol, SYMBOL_POINT) * 50;

    double slDistance = atrValue * ATR_SL_Mult;
    double tpDistance = atrValue * ATR_TP_Mult;

    // Dynamic lot sizing
    double lot = UseDynamicLots ?
        GetDynamicLot(slDistance, RiskPercent, strength) :
        NormalizeDouble(FixedBaseLot * GetLotMultiplier(strength), 2);

    // Validate lot
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    double step = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);

    lot = MathFloor(lot / step) * step;
    if(lot < minLot) lot = minLot;
    if(lot > maxLot) lot = maxLot;
    if(lot > MaxLotSize) lot = MaxLotSize;

    Print("========================================");
    Print("Opening ", MaxPositions, "x ", signal, " ", strength);
    Print("Score: ", score, " | Lot: ", DoubleToString(lot, 2));
    Print("ATR: ", DoubleToString(atrValue, _Digits));
    Print("SL Distance: ", DoubleToString(slDistance, _Digits));
    Print("TP Distance: ", DoubleToString(tpDistance, _Digits));
    Print("========================================");

    int successCount = 0;

    for(int i = 0; i < MaxPositions; i++) {
        double sl, tp;

        if(signal == "BUY") {
            sl = price - slDistance;
            tp = price + tpDistance;
        } else {
            sl = price + slDistance;
            tp = price - tpDistance;
        }

        if(!ValidateStops(price, sl, tp, signal)) {
            Print("✗ Invalid stops for level ", i+1);
            continue;
        }

        string comment = StringFormat("%s_%s_L%d_S%d",
            signal, strength, i+1, score);

        ulong ticket = OpenOrder(signal, lot, sl, tp, comment);

        if(ticket > 0) {
            int size = ArraySize(positions);
            ArrayResize(positions, size + 1);

            positions[size].ticket = ticket;
            positions[size].side = signal;
            positions[size].entryPrice = price;
            positions[size].lotSize = lot;
            positions[size].sl = sl;
            positions[size].tp = tp;
            positions[size].level = i + 1;
            positions[size].strength = strength;
            positions[size].entryScore = score;
            positions[size].beMovedTo = false;
            positions[size].trailingActive = false;
            positions[size].highest = price;
            positions[size].lowest = price;
            positions[size].beThreshold = GetBEThreshold(strength);
            positions[size].trailingStart = GetTrailingStart(strength);
            positions[size].trailingStep = GetTrailingStep(strength);
            positions[size].openTime = TimeCurrent();

            Print("✓ Opened L", i+1, " #", ticket);
            successCount++;
            stats.totalTrades++;
            stats.todayTrades++;
        } else {
            Print("✗ Failed L", i+1);
        }

        Sleep(200);
    }

    Print("Successfully opened ", successCount, "/", MaxPositions, " positions");
}

//==================== POSITION MANAGEMENT ===========================//
bool ModifyPosition(ulong ticket, double sl, double tp) {
    if(!PositionSelectByTicket(ticket)) return false;

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

    if(!OrderSend(request, result)) return false;

    return (result.retcode == TRADE_RETCODE_DONE);
}

void ManagePositions() {
    for(int i = ArraySize(positions) - 1; i >= 0; i--) {
        if(!PositionSelectByTicket(positions[i].ticket)) continue;

        double currentPrice = (positions[i].side == "BUY") ?
            SymbolInfoDouble(_Symbol, SYMBOL_BID) :
            SymbolInfoDouble(_Symbol, SYMBOL_ASK);

        if(positions[i].side == "BUY") {
            if(currentPrice > positions[i].highest)
                positions[i].highest = currentPrice;
        } else {
            if(currentPrice < positions[i].lowest)
                positions[i].lowest = currentPrice;
        }

        double profitPct = (positions[i].side == "BUY") ?
            (currentPrice - positions[i].entryPrice) / positions[i].entryPrice :
            (positions[i].entryPrice - currentPrice) / positions[i].entryPrice;

        // Breakeven logic (skip for STRONG/VERY_STRONG to let them run)
        bool skipBE = (positions[i].strength == "STRONG" ||
                       positions[i].strength == "VERY_STRONG");

        if(UseBreakeven && !skipBE && !positions[i].beMovedTo &&
           profitPct >= positions[i].beThreshold) {

            double newSL = (positions[i].side == "BUY") ?
                positions[i].entryPrice * (1 + BreakevenOffset) :
                positions[i].entryPrice * (1 - BreakevenOffset);

            newSL = NormalizeDouble(newSL, _Digits);

            bool shouldMove = (positions[i].side == "BUY" && newSL > positions[i].sl) ||
                              (positions[i].side == "SELL" && newSL < positions[i].sl);

            if(shouldMove && ModifyPosition(positions[i].ticket, newSL, positions[i].tp)) {
                positions[i].sl = newSL;
                positions[i].beMovedTo = true;
                Print("✓ BE set #", positions[i].ticket, " @", DoubleToString(newSL, _Digits));
            }
        }

        // Trailing stop
        if(UseTrailing && positions[i].beMovedTo &&
           profitPct >= positions[i].trailingStart) {

            double trailingSL;
            bool shouldModify = false;

            if(positions[i].side == "BUY") {
                trailingSL = positions[i].highest * (1 - positions[i].trailingStep);
                trailingSL = NormalizeDouble(trailingSL, _Digits);
                if(trailingSL > positions[i].sl + (_Point * 5))
                    shouldModify = true;
            } else {
                trailingSL = positions[i].lowest * (1 + positions[i].trailingStep);
                trailingSL = NormalizeDouble(trailingSL, _Digits);
                if(trailingSL < positions[i].sl - (_Point * 5))
                    shouldModify = true;
            }

            if(shouldModify && ModifyPosition(positions[i].ticket, trailingSL, positions[i].tp)) {
                positions[i].sl = trailingSL;
                Print("✓ Trailing #", positions[i].ticket, " -> ", DoubleToString(trailingSL, _Digits));
            }
        }
    }
}

//==================== POSITION SYNC =================================//
void SyncPositions() {
    // Remove closed positions
    for(int i = ArraySize(positions) - 1; i >= 0; i--) {
        if(!PositionSelectByTicket(positions[i].ticket)) {
            double profit = 0;

            if(HistorySelectByPosition(positions[i].ticket)) {
                for(int j = HistoryDealsTotal() - 1; j >= 0; j--) {
                    ulong dealTicket = HistoryDealGetTicket(j);
                    if(HistoryDealGetInteger(dealTicket, DEAL_POSITION_ID) == positions[i].ticket) {
                        profit += HistoryDealGetDouble(dealTicket, DEAL_PROFIT);
                    }
                }
            }

            stats.totalProfit += profit;
            stats.todayProfit += profit;

            if(profit > 0) {
                stats.winningTrades++;
                stats.consecutiveLosses = 0;
            } else if(profit < 0) {
                stats.losingTrades++;
                stats.consecutiveLosses++;
            }

            ArrayRemove(positions, i, 1);
        }
    }

    // Add external positions
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(ticket > 0 && PositionGetString(POSITION_SYMBOL) == _Symbol &&
           PositionGetInteger(POSITION_MAGIC) == MagicNumber) {

            bool found = false;
            for(int j = 0; j < ArraySize(positions); j++) {
                if(positions[j].ticket == ticket) {
                    found = true;
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
                positions[size].beThreshold = 0.0003;
                positions[size].trailingStart = 0.0005;
                positions[size].trailingStep = 0.0002;
                positions[size].openTime = (datetime)PositionGetInteger(POSITION_TIME);
                positions[size].entryScore = 0;
            }
        }
    }
}

//==================== UTILITY FUNCTIONS =============================//
int CountOpenPositions() {
    int count = 0;
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(ticket > 0 && PositionGetString(POSITION_SYMBOL) == _Symbol &&
           PositionGetInteger(POSITION_MAGIC) == MagicNumber) {
            count++;
        }
    }
    return count;
}

bool IsTradingSession() {
    MqlDateTime dt;
    TimeToStruct(TimeCurrent() + 7 * 3600, dt);
    int hour = dt.hour;

    if(UseAsianSession && hour >= 0 && hour < 8) return true;
    if(UseLondonSession && hour >= 8 && hour < 16) return true;
    if(UseNYSession && hour >= 16 && hour < 24) return true;

    return false;
}

bool IsNewsTime() {
    // Basic news filter - avoid XX:30 times (major news releases)
    MqlDateTime dt;
    TimeToStruct(TimeCurrent(), dt);
    return (dt.min >= 28 && dt.min <= 32);
}

bool CheckEquityStop() {
    if(!EnableEquityStop) return false;

    double balance = AccountInfoDouble(ACCOUNT_BALANCE);
    double equity = AccountInfoDouble(ACCOUNT_EQUITY);

    if(balance <= 0) return false;

    double drawdown = (balance - equity) / balance;
    if(drawdown > stats.maxDrawdown) stats.maxDrawdown = drawdown;

    return (drawdown >= MaxEquityDrawdown);
}

bool CheckDailyLossStop() {
    if(!EnableDailyLossStop) return false;
    return (stats.todayProfit <= -MathAbs(DailyLossLimit));
}

void UpdateStats() {
    double equity = AccountInfoDouble(ACCOUNT_EQUITY);
    if(equity > stats.peakEquity) stats.peakEquity = equity;

    // Reset daily stats at midnight
    static int lastDay = 0;
    MqlDateTime dt;
    TimeToStruct(TimeCurrent(), dt);

    if(dt.day != lastDay) {
        stats.todayTrades = 0;
        stats.todayProfit = 0;
        lastDay = dt.day;
    }
}

//==================== DISPLAY =======================================//
void UpdateDisplay() {
    MqlDateTime dt;
    TimeToStruct(TimeCurrent() + 7 * 3600, dt);

    string session = "CLOSED";
    string sessionColor = "🔴";

    if(dt.hour >= 0 && dt.hour < 8 && UseAsianSession) {
        session = "ASIAN"; sessionColor = "🟢";
    } else if(dt.hour >= 8 && dt.hour < 16 && UseLondonSession) {
        session = "LONDON"; sessionColor = "🟢";
    } else if(dt.hour >= 16 && dt.hour < 24 && UseNYSession) {
        session = "NY"; sessionColor = "🟢";
    }

    double price = iClose(_Symbol, PERIOD_M1, 0);
    int openPos = CountOpenPositions();

    double currentProfit = 0;
    for(int i = 0; i < ArraySize(positions); i++) {
        if(PositionSelectByTicket(positions[i].ticket)) {
            currentProfit += PositionGetDouble(POSITION_PROFIT);
        }
    }

    double winRate = (stats.totalTrades > 0) ?
        (stats.winningTrades * 100.0 / stats.totalTrades) : 0;

    string info = StringFormat(
        "╔════════════════════════════════════════╗\n"
        "║  SMART SCALPING BOT v4 PRO             ║\n"
        "╠════════════════════════════════════════╣\n"
        "║ Time: %02d:%02d:%02d (UTC+7)              ║\n"
        "║ Session: %s %s                        ║\n"
        "║ Price: %.5f                            ║\n"
        "║ ATR: %.5f | RSI: %.1f                  ║\n"
        "║ EMA: %.5f / %.5f                       ║\n"
        "╠════════════════════════════════════════╣\n"
        "║ Positions: %d / %d                      ║\n"
        "║ Multiple Signals: %s                    ║\n"
        "║ Last Signal: %s (Score: %d)             ║\n"
        "╠════════════════════════════════════════╣\n"
        "║ Current P/L: $%.2f                      ║\n"
        "║ Today P/L: $%.2f (%d trades)            ║\n"
        "║ Total P/L: $%.2f                        ║\n"
        "║ Win Rate: %.1f%% (%d/%d)                ║\n"
        "║ Max DD: %.2f%%                          ║\n"
        "║ Consecutive Losses: %d                  ║\n"
        "╚════════════════════════════════════════╝",
        dt.hour, dt.min, dt.sec,
        sessionColor, session,
        price,
        atr[0], rsi[0],
        emaFast[0], emaSlow[0],
        openPos, MaxTotalPositions,
        (AllowMultipleSignals ? "ON" : "OFF"),
        lastSignal, lastSignalScore,
        currentProfit,
        stats.todayProfit, stats.todayTrades,
        stats.totalProfit,
        winRate, stats.winningTrades, stats.totalTrades,
        stats.maxDrawdown * 100,
        stats.consecutiveLosses
    );

    Comment(info);
}