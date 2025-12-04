Here is the comprehensive **Technical Documentation and Update Guide** for the `AggressiveSwingBot_v1.mq5`.

This document is designed to help you understand the logic, configure the bot, and provides a roadmap for future updates/optimizations.

---

# 📘 Aggressive Swing Bot v1.0 - Technical Documentation

**⚠️ CRITICAL WARNING: HIGH RISK ALGORITHM**
This Expert Advisor is configured by default to risk **50% of the account balance per trade**. A string of 2 consecutive losses will result in a 75% drawdown. **Do not run this on a live account without significantly lowering the risk settings.**

---

## 1. Strategy Overview
**Type:** Trend Following / Swing Trading
**Timeframe:** H4 (4-Hour) Default
**Execution:** Market Execution with Dynamic Lot Sizing based on ATR.

### Entry Logic
A trade is triggered only when **ALL** of the following conditions are met:

1.  **Trend Filter:**
    *   **Buy:** Price is *above* the 200 EMA.
    *   **Sell:** Price is *below* the 200 EMA.
2.  **Trend Strength:**
    *   ADX (14) is greater than `ADX_MinStrength` (20).
3.  **Signal Trigger (Either A or B):**
    *   **A. EMA Crossover:** Fast EMA (20) crosses Slow EMA (50).
    *   **B. RSI Reversal:** RSI exits Oversold (<30) for Buy, or exits Overbought (>70) for Sell.

### Exit Logic
1.  **Stop Loss:** Fixed at Entry ± (ATR × 2.0).
2.  **Take Profit:** Fixed at Entry ± (ATR × 4.0) (Risk:Reward 1:2).
3.  **Breakeven:** Moves SL to Entry Price when price moves (ATR × 1.0) in profit.
4.  **Trailing Stop:** Trails price by (ATR × 1.5) once the Breakeven is active.

---

## 2. Input Parameters Guide

### 🔴 Group: Aggressive Risk Settings
| Parameter | Default | Description |
| :--- | :--- | :--- |
| `RiskPercentPerSignal` | **50.0** | Percentage of Account Balance to lose if SL is hit. |
| `MaxPositionsPerSignal`| 1 | Limits the bot to one trade at a time. |
| `MaxLotSize` | 100.0 | Hard cap on volume to prevent broker rejection. |

### 📈 Group: Swing Trading Settings
| Parameter | Default | Description |
| :--- | :--- | :--- |
| `SwingTimeframe` | PERIOD_H4 | The timeframe used for calculation (not the chart timeframe). |
| `EMA_Fast` | 20 | The triggering moving average. |
| `EMA_Slow` | 50 | The baseline moving average. |
| `EMA_Trend` | 200 | The long-term trend filter (Bull/Bear line). |
| `ADX_MinStrength` | 20 | Minimum volatility required to trade. |

### 🛡️ Group: Management
| Parameter | Default | Description |
| :--- | :--- | :--- |
| `TradingStartHour` | 0 | Start trading time (Broker Server Time). |
| `TradingEndHour` | 18 | Stop trading time (Broker Server Time). |
| `MagicNumber` | 888999 | Unique ID to distinguish this bot's trades. |

---

## 3. Development Roadmap (Update Plan)

If you plan to update this EA to **v1.1** or **v2.0**, strictly follow this roadmap to improve stability and profitability while managing the extreme risk profile.

### Phase 1: Risk Management Optimization (Critical)
The current 50% risk is unsustainable mathematically.
*   **Update Idea:** Implement **Martingale or Gridding Recovery**.
    *   *Logic:* If a trade hits SL, the next trade increases lot size to recover the loss.
*   **Update Idea:** Implement **Equity Protection**.
    *   *Logic:* Hard stop trading if Daily Drawdown > X% or Total Drawdown > Y%.
*   **Code Change:**
    ```cpp
    // Add to Inputs
    input double MaxDailyLossPercent = 20.0;
    // Logic in OnTick to check AccountEquity vs StartOfDayEquity
    ```

### Phase 2: Signal Filtering
To reduce false signals during choppy markets:
*   **Update Idea:** **Multi-Timeframe Analysis (MTF)**.
    *   *Logic:* Only Buy on H4 if Daily (D1) Trend is also Up.
*   **Update Idea:** **Candle Pattern Confirmation**.
    *   *Logic:* Wait for a specific candle close (e.g., Bullish Engulfing) after the EMA crossover before entering.

### Phase 3: Execution Logic
*   **Update Idea:** **Retry Logic**.
    *   *Current Issue:* If `OrderSend` fails (requotes/slippage), the bot gives up.
    *   *Fix:* Add a `while` loop to retry the order 3 times with a small sleep interval.
*   **Update Idea:** **Spread Filter**.
    *   *Logic:* Do not trade if `SymbolInfoInteger(_Symbol, SYMBOL_SPREAD)` > MaxSpread.

### Phase 4: User Interface (UI)
*   **Update Idea:** **On-Chart Dashboard**.
    *   Instead of using `Comment()`, use `CreateLabel()` objects to make a nice button panel to manually Close All or Pause the bot.

---

## 4. Troubleshooting Common Errors

| Status Message | Cause | Solution |
| :--- | :--- | :--- |
| **"Error 131 (ERR_INVALID_TRADE_VOLUME)"** | Calculated Lot Size is too high or low. | Check if 50% risk results in a lot size larger than `MaxLotSize`. Check Account Balance. |
| **"Error 4756 (ERR_TRADE_SEND_FAILED)"** | Request failed. | Often caused by `ORDER_FILLING_IOC`. Some brokers require `ORDER_FILLING_FOK`. |
| **"OUTSIDE TRADING HOURS"** | Server time is > 18:00. | Adjust `TradingEndHour` or check your Broker's server time zone. |
| **"Invalid ATR"** | Indicator handle failed. | Ensure history data for the H4 timeframe is downloaded. |

---

## 5. Deployment Checklist
1.  **Backtest:** Run continuously in Strategy Tester for at least 1 year of data.
2.  **Demo:** Run on a Demo account for 2 weeks to verify entry/exit execution.
3.  **Broker Check:** Ensure your broker allows **Hedging** (though this bot uses one position, conflict may occur if you trade manually) and verify the **Leverage** (50% risk requires high leverage).
4.  **VPS:** Install on a Virtual Private Server (VPS) for 24/7 uptime to ensure Trailing Stops work correctly.