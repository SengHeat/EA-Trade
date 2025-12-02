Here is the comprehensive documentation for **Smart Scalping Bot v3.20**. This document explains the strategy, money management logic, and input parameters.

---

# 📘 Smart Scalping Bot v3.20 – Documentation

## 1. Overview
**Smart Scalping Bot v3.20** is a fully automated Expert Advisor (EA) for MQL5 designed for safe trend scalping on the **M1 Timeframe**. It uses a weighted scoring system based on multiple indicators (EMA, RSI, Bollinger Bands, MACD) to determine trade quality.

**New in v3.20:**
*   **Lot Size Limits:** Hard limits set to Minimum **0.1** and Maximum **10.0**.
*   **Score-Based Sizing:** Position size increases dynamically based on the strength of the signal score.
*   **Balance-Based Calculation:** Risk is calculated on Account Balance (not Equity).

---

## 2. Trading Strategy Logic

The bot analyzes the market on the **M1 timeframe** and calculates a **Signal Score** (0 to 12). A trade is only taken if the score is **≥ 5**.

### Signal Scoring Components
1.  **EMA Crossover (3 points):** Fast EMA crosses Slow EMA.
2.  **RSI Filter (2 points):** Buy if RSI < 40 (Oversold), Sell if RSI > 60 (Overbought).
3.  **Bollinger Bands (2 points):** Checks for price interaction with bands and band width.
4.  **Trend Alignment (1 point):** Fast EMA is above/below Slow EMA.
5.  **MACD (1 point):** Main line crossing signal line.

### Higher Timeframe Filter
*   **TrendTF1 (M15) & TrendTF2 (H1):** If `RequireHigherTFTrend` is true, the bot will only Buy if the trend on M15 and H1 is bullish, and Sell if bearish.

---

## 3. Money Management (New & Critical)

This version uses an advanced dynamic lot sizing algorithm.

### A. Lot Size Limits
Regardless of account size, the bot enforces these hard limits:
*   **Minimum Lot:** 0.10
*   **Maximum Lot:** 10.00

### B. Dynamic Calculation Formula
1.  **Base Risk:** Calculates risk amount based on **Account Balance** and `RiskPercent` input (Default 2.0%).
2.  **Signal Multiplier:** The lot size is multiplied based on the **Signal Score**:
    *   **Score 5-6 (Medium):** 1.0x Risk
    *   **Score 7-8 (Strong):** 1.5x Risk
    *   **Score 9+ (Very Strong):** 2.0x Risk
3.  **Stop Loss Scaling:** The final lot size is adjusted so that if the Stop Loss is hit, the loss equals the calculated risk amount.

---

## 4. Execution Modes

You can choose how the bot enters trades via the `ExecutionMode` input:

1.  **MODE_INSTANT:** Enters immediately at current market price.
2.  **MODE_PENDING_LIMIT:** Places Buy Limit (below price) or Sell Limit (above price). Best for catching pullbacks.
3.  **MODE_PENDING_STOP:** Places Buy Stop (above price) or Sell Stop (below price). Best for breakouts.
4.  **MODE_HYBRID_LIMIT:** Places 1 Market trade immediately AND 1 Limit order pending at a better price.

---

## 5. Input Parameters Glossary

### === Trading Settings ===
*   **ExecutionMode:** Choose between Instant, Limit, Stop, or Hybrid.
*   **MaxPositions:** Max trades per specific signal (Default: 3).
*   **MaxTotalPositions:** Max trades allowed on the account total (Default: 20).
*   **AllowMultipleSignals:** If true, allows new trade setups even if old trades are open.

### === Pending Order Settings ===
*   **PendingDistanceATR:** How far away (in ATR) to place pending orders.
*   **PendingExpirationMin:** Minutes before a pending order is deleted if not filled.
*   **DeletePendingOnOpposite:** Removes Buy Limits if a Sell signal appears (and vice versa).

### === Money Management ===
*   **UseDynamicLots:** `true` = use Balance/Score logic; `false` = use Fixed Lots.
*   **RiskPercent:** Percentage of **Balance** to risk per setup (Default: 2.0).
*   **MinLotSize:** Floor for trade size (Default: 0.1).
*   **MaxLotSize:** Ceiling for trade size (Default: 10.0).
*   **FixedBaseLot:** Used only if Dynamic Lots is `false`.

### === Risk Management ===
*   **Slippage:** Max allowed price deviation.
*   **UseBreakeven:** Moves SL to entry price after a certain profit.
*   **UseTrailing:** Trails the Stop Loss behind profit.
*   **DailyLossLimit:** Pauses trading if daily loss exceeds this ($).
*   **MaxEquityDrawdown:** Pauses trading if equity drops by this % (0.10 = 10%).

### === Volatility & Filters ===
*   **ATR_Period:** Used to calculate dynamic Stop Loss and Take Profit distances.
*   **ATR_SL_Mult:** Stop Loss distance = ATR × 2.5.
*   **ATR_TP_Mult:** Take Profit distance = ATR × 3.5.

---

## 6. Installation Guide

1.  Open MetaTrader 5 (MT5).
2.  Go to **File -> Open Data Folder**.
3.  Navigate to `MQL5` -> `Experts`.
4.  Copy the `SmartScalpingBot_v3_Pending.mq5` file into this folder.
5.  Restart MT5 or Right-click "Experts" in the Navigator and select "Refresh".
6.  Drag the bot onto an **M1 Chart** (e.g., XAUUSD M1 or EURUSD M1).
7.  Ensure "Algo Trading" is enabled in the toolbar.

---

## 7. Disclaimer
*   **Risk Warning:** Trading foreign exchange and CFDs carries a high level of risk and may not be suitable for all investors.
*   **Testing:** Always test this bot on a **Demo Account** first to understand how the Scoring and Dynamic Lot sizing works with your specific broker's spread and conditions.