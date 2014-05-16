//
//  Pricer.cpp
//
//  Created by James Ma on 3/14/14.
//
//  In response to the problem posted at:
//  http://rgmadvisors.com/problems/orderbook/
//
//  Using pointers, pass-by-reference and public libraries makes 
//  development and code execution fast, while minimizing overhead.
//
//  Add Order message complexity is O(n). Reduce Order message complexity is O(n).
//
//  If production code is slow, look for potential bottlenecks:
//  input method, type conversion method, variables requiring memory chunks,
//  and storage method (vectors, array, map, etc). Time the run of each function in unit tests.
//  Other possible bottlenecks - sorting algo and searching algo.
//  Need to evaulate trade-off between faster execution and more memory usage.
//

#include <iostream>
#include <fstream>
#include <iostream>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

const char* BUY = "B";
const char* SELL = "S";

const char* ORDER_TYPE_ADD = "A";
const char* ORDER_TYPE_REMOVE = "R";

using namespace std;

class Tick
{
	string timestamp;
	string orderId;
	double price;
	int orderSize;
public:
    Tick (string, string, double,int);
	string getTimestamp(){ return timestamp; }
	string getOrderId(){ return orderId; }
	double getPrice(){ return price; }
	int getOrderSize(){ return orderSize; }
	void setOrderSize(int _orderSize){ orderSize = _orderSize; }
};

Tick::Tick(string _timestamp, string _orderId, double _price, int _orderSize)
{
	timestamp = _timestamp;
	orderId = _orderId;
	price = _price;
	orderSize = _orderSize;
}

class OrderBook
{
	int targetSize;
	int totalBidQty;
	int totalAskQty;
	int position;
	double sellIncome;
	double buyIncome;
	vector<Tick> bids;
	vector<Tick> asks;
	
public:
	OrderBook(int);

	void addBid(Tick &tick)
	{
		totalBidQty += tick.getOrderSize();
		addTickToOrderBook(bids, tick);
		checkAndSellTargetShares(bids, tick.getTimestamp(), totalBidQty);
	}
	
	void addAsk(Tick &tick)
	{
		totalAskQty += tick.getOrderSize();
		addTickToOrderBook(asks, tick);
		checkAndBuyTargetShares(asks, tick.getTimestamp(), totalAskQty);
	}
			
	void reduceOrder(string &timestamp, string &orderId, int qty)
	{
		if (reduceOrderOnBookSide(bids, orderId, qty))
		{
			totalBidQty = max(0, totalBidQty-qty);
			checkAndSellTargetShares(bids, timestamp, totalBidQty);
		}
		else if (reduceOrderOnBookSide(asks, orderId, qty))
		{
			totalAskQty = max(0, totalAskQty-qty);
			checkAndBuyTargetShares(asks, timestamp, totalAskQty);
		}
	}
	
private:
	void addTickToOrderBook(vector<Tick> &orders, Tick &tick)
	{
		bool isAdded = false;
		for(int i = 0; i<(int)orders.size(); i++)
		{
			if (orders[i].getPrice() > tick.getPrice())
			{
				isAdded = true;
				orders.insert(orders.begin()+i, tick);
				return;
			}
		}
		
		if (!isAdded)
			orders.push_back(tick);
	}
	
	bool reduceOrderOnBookSide(vector<Tick> &orders, string &orderId, int &qty)
	{
		for(vector<Tick>::iterator tick=orders.begin(); tick!=orders.end();++tick)
		{
			if (tick->getOrderId().compare(orderId) == 0)
			{
				int storedQty = tick->getOrderSize();
				if (storedQty <= qty)
					orders.erase(tick);
				else
					tick->setOrderSize(storedQty - qty);
				return true;
			}
		}
		return false;
	}
	
	void checkAndSellTargetShares(vector<Tick> &orders, string timestamp, int &totalBookQty)
	{
		bool isAsksHaveTargetSize = (totalBookQty >= targetSize);
		if (isAsksHaveTargetSize)
		{
			sellIncome = 0.00;
			int workingSize = targetSize;

			for(vector<Tick>::iterator tick=orders.end(); tick!=orders.begin();)
			{
				--tick;
				addToPnl(tick, workingSize, sellIncome);
				if (workingSize == 0)
					break;
			}
			
			position = -targetSize;
			printOutput(sellIncome, SELL, timestamp);
			return;
		}
		
		bool isPnlAffected = ((sellIncome>0) && (totalBookQty < targetSize));
		if (isPnlAffected)
		{
			sellIncome = 0.00;
            printOutput(sellIncome, SELL, timestamp);
		}
	}
	
	void checkAndBuyTargetShares(vector<Tick> &orders, string timestamp, int &totalBookQty)
	{
		bool isBidsHaveTargetSize = (totalBookQty >= targetSize);
		if (isBidsHaveTargetSize)
		{
			buyIncome = 0.00;
			int workingSize = targetSize;
			
			for(vector<Tick>::iterator tick=orders.begin(); tick!=orders.end();++tick)
			{
				addToPnl(tick, workingSize, buyIncome);
				if (workingSize == 0)
					break;
			}

			position = targetSize;			
			printOutput(buyIncome, BUY, timestamp);			
			return;
		}
		
		bool isPnlAffected = ((buyIncome>0) && (totalBookQty < targetSize));
		if (isPnlAffected)
		{
			buyIncome = 0.00;			
            printOutput(buyIncome, BUY, timestamp);
		}
	}
    
    void printOutput(double price, const char* command, string &timestamp)
    {
        cout << timestamp << " " << command << " ";
        (price==0) ? printf("NA") : printf("%.2f", price);
        cout << endl;
    }
	
	int addToPnl(vector<Tick>::iterator &tick, int &workingSize, double &pnl)
	{
		int currentSize = tick->getOrderSize();
		int effectiveSize =  ((currentSize <= workingSize) ? currentSize : workingSize);
		pnl += effectiveSize * tick->getPrice();
		workingSize -= effectiveSize;
		return effectiveSize;
	}
};

OrderBook::OrderBook(int _targetSize)
{
	targetSize = _targetSize;
	totalBidQty = 0;
	totalAskQty = 0;
	position = 0;
	sellIncome = 0;
	buyIncome = 0;
}

bool isBidSide(string value)
{
	return (value.compare(BUY) == 0);
}

bool isAskSide(string value)
{
	return (value.compare(SELL) == 0);
}

bool isAddOrder(string value)
{
	return (value.compare(ORDER_TYPE_ADD) == 0);
}

bool isReduceOrder(string value)
{
	return (value.compare(ORDER_TYPE_REMOVE) == 0);
}

void processAddOrder(string &side, Tick &tick, OrderBook &orderBook)
{
	if (isBidSide(side))
		orderBook.addBid(tick);
	else if (isAskSide(side))
		orderBook.addAsk(tick);
}

void processDataFeed(string data, OrderBook &orderBook)
{
	vector<string> tokens;
    split(tokens, data, boost::	is_any_of(" "));
	if (tokens.size() >= 3)
	{
		string timestamp = tokens[0];
		string message = tokens[1];
		string orderId = tokens[2];
		
		if (isAddOrder(message))
		{
			string side = tokens[3];
			double price = atof(tokens[4].c_str());
			int size = atoi(tokens[5].c_str());
			
			Tick tick(timestamp, orderId, price, size);
			processAddOrder(side, tick, orderBook);
		}
		else if (isReduceOrder(message))
		{
			int qtyToRedudce = atoi(tokens[3].c_str());
			orderBook.reduceOrder(timestamp, orderId, qtyToRedudce);
		}
	}
}

int main(int argc, const char * argv[])
{
    const char* DEFAULT_PRICER_FILE = "pricer.in";
    int TARGET_SIZE_ARGV_INDEX = 1;
    int FILE_NAME_ARGV_INDEX = 2;
    
    bool isArgumentContainsFileName = (argc==(FILE_NAME_ARGV_INDEX+1));
    bool isArgumentContainsTargetSize = (argc==(TARGET_SIZE_ARGV_INDEX+1));

	if (!(isArgumentContainsTargetSize || isArgumentContainsFileName))
	{
		cerr << "Command: Pricer target-size [file-name]" << endl;
		return 0;
	}
	
	int targetSize = atoi(argv[TARGET_SIZE_ARGV_INDEX]);
    const char* targetFile = isArgumentContainsFileName ? 
                                            argv[FILE_NAME_ARGV_INDEX] : DEFAULT_PRICER_FILE;
    
    OrderBook orderBook(targetSize);
	ifstream pricerFile;
    string data;
    
	pricerFile.open(targetFile);
	if (!pricerFile.is_open()){
		cout << "Can't open input file.\n";
		exit(1);
	}
	else{
		while (!pricerFile.eof())
		{
			getline(pricerFile, data);
			processDataFeed(data, orderBook);
		}
	}
	pricerFile.close();
	
    return 0;
}
