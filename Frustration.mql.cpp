#property copyright "Copyright 2017, MetaQuotes Software Corp."
#property link      "https://www.mql5.com"
#property version   "1.00"
#property strict

// Input variables
extern int magic_numb = 0;
extern double lots    = 0.01;
extern double tp_pips = 100;
extern double sl_pips = 100;
extern int stoch_K = 5;
extern int stoch_slow = 3;
extern int stoch_D = 3;
extern int stoch_max = 80;
extern int stoch_min = 20;
extern int cci_period = 14;
extern int cci_max = 100;
extern int cci_min = -100;
extern int mfi_period = 14;
extern int mfi_max = 80;
extern int mfi_min = 20;

// Internal state variables
bool buy_trade  = false;
bool sell_trade = false;
bool in_trade   = false;
bool prev_trade = false;
double commission   = 0;
double prev_balance = 0;
double price        = 0;
double profit       = 0;
double stoploss     = 0;
double swap         = 0;
double takeprofit   = 0;
int error  = 0;
int points = 0;
int total_trades = 0;
int ticket = 0;

int OnInit()
{
    UpdateState();
    return(INIT_SUCCEEDED);
}

void OnDeinit(const int reason) {}

double OnTester()
{
    return points;
}

void OnTick()
{
    UpdateState();

    if (!in_trade)
    {
        double stoch_curr = iStochastic(NULL, 0, stoch_K, stoch_D, stoch_slow, MODE_SMA, 0, MODE_MAIN, 0);
        double stoch_prev = iStochastic(NULL, 0, stoch_K, stoch_D, stoch_slow, MODE_SMA, 0, MODE_MAIN, 1);
        double stoch_sign = iStochastic(NULL, 0, stoch_K, stoch_D, stoch_slow, MODE_SMA, 0, MODE_SIGNAL, 0);
        double cci = iCCI(NULL, 0, cci_period, PRICE_TYPICAL, 0);
        double mfi = iMFI(NULL, 0, mfi_period, 0);

        if (stoch_curr > stoch_sign && stoch_prev < stoch_sign && stoch_curr < stoch_min && cci < cci_min && mfi < mfi_min) SendOrder(OP_BUY);
        if (stoch_curr < stoch_sign && stoch_prev > stoch_sign && stoch_curr > stoch_max && cci > cci_max && mfi > mfi_max) SendOrder(OP_SELL);
    }
}

void UpdateState()
{
    commission = 0;
    profit     = 0;
    swap       = 0;
    buy_trade  = false;
    sell_trade = false;
    in_trade   = false;
    error      = 0;
    price      = 0;
    stoploss   = 0;
    takeprofit = 0;

    // Loop through all the orders to find ours
    for (int i = 0; i < OrdersTotal(); i++)
    {
        OrderSelect(i, SELECT_BY_POS, MODE_TRADES);

        if (OrderMagicNumber() == magic_numb)
        {
            commission = OrderCommission();
            profit     = OrderProfit();
            price      = OrderOpenPrice();
            swap       = OrderSwap();
            in_trade   = true;
            takeprofit = OrderTakeProfit();
            stoploss   = OrderStopLoss();
            ticket     = OrderTicket();

            if (OrderType() == OP_BUY )
            {
                buy_trade  = true;
                sell_trade = false;
            }
            if (OrderType() == OP_SELL)
            {
                buy_trade = false;
                sell_trade = true;
            }
            break;
        }
    }

    // Trailing stop
    if (in_trade)
    {
        if (buy_trade)
        {
            if (Bid >= price + MarketInfo(NULL, MODE_STOPLEVEL) * Point)
            {
                double new_stoploss = NormalizeDouble(Bid - MarketInfo(NULL, MODE_STOPLEVEL) * Point, Digits);
                if (new_stoploss > stoploss)
                {
                    OrderModify(ticket, 0, new_stoploss, takeprofit, 0, 0);
                }
            }
        }
        if (sell_trade)
        {
            if (Ask <= price - MarketInfo(NULL, MODE_STOPLEVEL) * Point)
            {
                double new_stoploss = NormalizeDouble(Ask + MarketInfo(NULL, MODE_STOPLEVEL) * Point, Digits);
                if (new_stoploss < stoploss)
                {
                    OrderModify(ticket, 0, new_stoploss, takeprofit, 0, 0);
                }
            }
        }
    }

    // Count trades ended in profit as points for testing
    if (!in_trade && prev_trade)
    {
        total_trades++;
        if (AccountBalance() >= prev_balance)
        {
            points++;
        }
    }
    prev_trade   = in_trade;
    prev_balance = AccountBalance();

    // Print state variables
    Comment(
           "\nProfit: " + DoubleToStr(profit, 2) + " - " +
           "Trades: "   + DoubleToStr(total_trades, 0)     + " - " +
           "Score: "    + DoubleToStr(points, 0)    + " - " +
           "In Trade: " + DoubleToStr(in_trade, 0)   + " - " +
           "Time: "     + IntegerToString(TimeCurrent() - Time[0])
           );
}

void SendOrder(int OP_TYPE)
{
    int slip = 100;
    string comment = "";

    if (OP_TYPE == OP_SELL)
    {
        buy_trade  = false;
        sell_trade = true;

        double take_profit = Ask - (tp_pips * Point);
        double stop_loss   = Ask + (sl_pips * Point);

        error = OrderSend(Symbol(), OP_TYPE, lots, Bid, slip, stop_loss,
                          take_profit, comment, magic_numb, 0, clrHotPink);
    }
    if (OP_TYPE == OP_BUY)
    {
        buy_trade  = true;
        sell_trade = false;

        double take_profit = Bid + (tp_pips * Point);
        double stop_loss   = Bid - (sl_pips * Point);

        error = OrderSend(Symbol(), OP_TYPE, lots, Ask, slip, stop_loss,
                          take_profit, comment, magic_numb, 0, clrLimeGreen);
    }
}
