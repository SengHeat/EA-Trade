Here is the complete documentation for **Smart Scalping Bot v3.1 (Pending Order Edition)**.

---

# 📘 Smart Scalping Bot v3.1 - Documentation

## 1. Overview
This is a **Trend-Following M1 Scalper** designed for high-frequency pairs (like XAUUSD, GBPJPY, EURUSD). It uses a "Voting System" based on 5 indicators (EMA, RSI, Bollinger Bands, MACD, Volume) to generate a **Signal Score (0-10)**.

Unlike standard scalpers that only buy/sell instantly, Version 3.1 introduces **Execution Modes**, allowing you to trade via Breakouts (Stop Orders), Retracements (Limit Orders), or Instant execution.

---

## 2. Installation
1.  **File:** Save the code as `SmartScalpingBot_v3_Pending.mq5`.
2.  **Folder:** Move it to `MQL5/Experts/`.
3.  **Compile:** Open MetaEditor (F4), click **Compile**.
4.  **Chart:** Open an **M1 Chart** (1-Minute Timeframe).
5.  **Algo Trading:** Ensure the "Algo Trading" button is Green (ON).

---

## 3. Execution Modes (New Feature)
This is the most important setting under `=== Trading Settings ===`.

| Mode | Description | Best Market Condition |
| :--- | :--- | :--- |
| **MODE_INSTANT** | Enters the market immediately when a signal is found. | Fast-moving trends. |
| **MODE_PENDING_LIMIT** | Places a Limit Order *behind* current price (Buy Low). If price pulls back, it triggers. | Choppy/Ranging markets or healthy trends with pullbacks. |
| **MODE_PENDING_STOP** | Places a Stop Order *ahead* of current price (Buy High). Triggers only if momentum continues. | Breakout trading (News/High Volatility). |
| **MODE_HYBRID_LIMIT** | Opens **1 Market Trade** immediately AND places **1 Limit Order** behind it to average down. | "Grid-style" recovery. Use with caution. |

---

## 4. Input Parameters Explained

### A. Trading Settings
*   **ExecutionMode**: Selects how the bot enters trades (see above).
*   **MaxPositions**: How many separate trades to open per signal.
*   **MaxTotalPositions**: Hard cap on total open trades (Active + Pending).
*   **AllowMultipleSignals**:
    *   `true`: Can open new trades if a new signal appears 5 minutes later.
    *   `false`: Must close all current trades before opening new ones.

### B. Pending Order Settings
*   **PendingDistanceATR**: How far away to place the pending order.
    *   *Example:* If ATR is 10 pips and this is `0.5`, Limit order is placed 5 pips away.
*   **PendingExpirationMin**: How long (minutes) a pending order waits before being deleted.
*   **DeletePendingOnOpposite**: If a BUY Limit is waiting, but a SELL signal appears, delete the BUY Limit.

### C. Money Management (Dynamic Lots)
*   **UseDynamicLots**:
    *   `true`: Auto-calculates lot size based on account equity and Stop Loss distance.
    *   `false`: Uses `FixedBaseLot`.
*   **RiskPercent**: % of Equity to risk per trade setup.
    *   *Formula:* `(Equity * Risk%) / MaxPositions`.
*   **MaxLotSize**: Safety cap (e.g., never trade more than 5.0 lots).

### D. Indicator Settings (The "Voting" Logic)
The bot calculates a score (0 to 10). It only trades if `Score >= MinSignalScore`.
*   **EMA_Fast/Slow**: Detects trend direction and crossovers (3 points).
*   **RSI**: Detects Oversold/Overbought conditions (2 points).
*   **Bollinger Bands**: Detects price rejection at outer bands (2 points).
*   **MACD**: Confirms momentum (1 point).

### E. Risk Management (Stop Loss & Take Profit)
The SL and TP are **not fixed pips**. They expand and contract based on Market Volatility (ATR).
*   **ATR_Period**: Measure volatility over last 14 candles.
*   **ATR_SL_Mult**: Stop Loss distance multiplier.
    *   *Higher (3.0+)* = Safer, wider stop.
    *   *Lower (1.5)* = Aggressive, tight stop.
*   **ATR_TP_Mult**: Take Profit distance multiplier.

### F. Safety Filters
*   **DailyLossLimit**: If the bot loses $500 (or your setting) today, it stops until tomorrow.
*   **MaxEquityDrawdown**: If equity drops 10% below balance, it emergency stops.
*   **RequireHigherTFTrend**:
    *   `true`: If M1 says BUY, it checks M15 and H1. If they are SELL, it **BLOCKS** the trade.
    *   `false`: Trades strictly based on M1 signals.

---

## 5. Troubleshooting (Why is it not trading?)

If the bot is on the chart (Smiley face in top right) but not opening trades, check the **Experts** tab at the bottom of MT5.

| Error / Log Message | Solution |
| :--- | :--- |
| **"Blocked by HTF trend"** | You are trying to Buy against the H1 trend. Set `RequireHigherTFTrend = false` to scalp reversals. |
| **"Blocked: ATR too small"** | The market is dead/flat. The bot is saving you from spread costs. Wait for volume. |
| **"Too soon (wait X bars)"** | `MinBarsBetweenSignals` is preventing spamming. Wait or lower that setting. |
| **"Equity stop triggered"** | You hit your `MaxEquityDrawdown`. Restart MT5 or reset the EA to resume. |
| **Pending Orders never trigger** | Your `PendingDistanceATR` is too high (e.g., 1.0). Lower it to `0.3` or `0.5` to catch smaller pullbacks. |

---

## 6. Recommended Configurations

### Profile A: "Safe Trend Follower" (Low Risk)
*   **ExecutionMode:** `MODE_INSTANT`
*   **RiskPercent:** `1.0`
*   **RequireHigherTFTrend:** `true` (Must agree with H1)
*   **MinSignalScore:** `5` (Only high quality signals)
*   **ATR_SL_Mult:** `2.5` / **TP:** `3.5`

### Profile B: "Aggressive Scalper" (High Frequency)
*   **ExecutionMode:** `MODE_INSTANT`
*   **RiskPercent:** `2.0`
*   **RequireHigherTFTrend:** `false` (Ignore H1, trade M1 movements)
*   **MinSignalScore:** `2` (Trade almost every crossover)
*   **ATR_SL_Mult:** `1.5` (Tight stops) / **TP:** `2.5`

### Profile C: "Limit Entry / Swing" (Precision)
*   **ExecutionMode:** `MODE_PENDING_LIMIT`
*   **PendingDistanceATR:** `0.4`
*   **PendingExpirationMin:** `30`
*   **RiskPercent:** `1.5`
*   **RequireHigherTFTrend:** `true`

---

## 7. Important Warnings
1.  **Timeframe:** This bot is coded for **M1 (1-Minute)** charts. Attaching it to H1 will still calculate M1 logic (unless you modified the code as per the previous instruction).
2.  **Magic Number:** If you run this on multiple pairs (e.g., Gold and Euro), change the `MagicNumber` for each one (e.g., 234000, 234001) so the bot doesn't get confused managing trades.
3.  **Spread:** Do not use this on pairs with high spreads. The ATR Stop Loss might be smaller than the spread, causing immediate losses. **Use Low Spread (ECN) accounts only.**