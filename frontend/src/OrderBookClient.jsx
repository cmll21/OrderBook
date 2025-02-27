// OrderBookClient.jsx
import React, { useState, useEffect } from 'react';
import { Line } from 'react-chartjs-2';
import 'chart.js/auto';

// Time-series chart component for mid prices.
const OrderBookTimeSeriesChart = ({ midPriceSeries }) => {
    const data = {
        labels: midPriceSeries.map(pt => pt.time),
        datasets: [
            {
                label: 'Mid Price',
                data: midPriceSeries.map(pt => pt.midPrice),
                borderColor: 'blue',
                backgroundColor: 'rgba(0, 0, 255, 0.2)',
                fill: false,
            },
        ],
    };

    const options = {
        responsive: true,
        scales: {
            x: { title: { display: true, text: 'Time' } },
            y: { title: { display: true, text: 'Price' } },
        },
    };

    return (
        <div style={{ width: '100%', height: '300px' }}>
            <Line data={data} options={options} />
        </div>
    );
};

// Depth chart component for one side of the order book.
const OrderBookDepth = ({ levels, side }) => {
    const maxVolume =
        levels && levels.length > 0 ? Math.max(...levels.map(level => level.quantity)) : 1;
    const barColor = side === 'bid' ? 'rgba(0, 200, 0, 0.3)' : 'rgba(200, 0, 0, 0.3)';

    return (
        <div>
            {levels &&
                levels.map((level, idx) => {
                    const widthPercent = (level.quantity / maxVolume) * 100;
                    return (
                        <div
                            key={idx}
                            style={{
                                position: 'relative',
                                margin: '2px 0',
                                height: '24px',
                                lineHeight: '24px',
                                fontSize: '14px',
                                backgroundColor: '#f7f7f7',
                            }}
                        >
                            <div
                                style={{
                                    position: 'absolute',
                                    top: 0,
                                    bottom: 0,
                                    left: side === 'bid' ? 0 : 'auto',
                                    right: side === 'ask' ? 0 : 'auto',
                                    width: `${widthPercent}%`,
                                    backgroundColor: barColor,
                                }}
                            />
                            <div
                                style={{
                                    position: 'relative',
                                    paddingLeft: side === 'bid' ? '5px' : 0,
                                    paddingRight: side === 'ask' ? '5px' : 0,
                                    zIndex: 1,
                                    textAlign: side === 'bid' ? 'left' : 'right',
                                }}
                            >
                                {`Price: ${level.price} | Qty: ${level.quantity}`}
                            </div>
                        </div>
                    );
                })}
        </div>
    );
};

// Metrics panel component that displays key calculated metrics.
const MetricsPanel = ({ metrics }) => {
    if (!metrics) return <div>No metrics available.</div>;
    const { bestBid, bestAsk, spread, midPrice, totalBidVolume, totalAskVolume, volatility } = metrics;
    return (
        <div style={{ marginBottom: '1rem', border: '1px solid #ccc', padding: '1rem' }}>
            <h3>Order Book Metrics</h3>
            <p>
                <strong>Best Bid:</strong> {bestBid}
            </p>
            <p>
                <strong>Best Ask:</strong> {bestAsk}
            </p>
            <p>
                <strong>Spread:</strong> {spread}
            </p>
            <p>
                <strong>Mid Price:</strong> {midPrice}
            </p>
            <p>
                <strong>Total Bid Volume:</strong> {totalBidVolume}
            </p>
            <p>
                <strong>Total Ask Volume:</strong> {totalAskVolume}
            </p>
            <p>
                <strong>Volatility (std. dev of mid price):</strong> {volatility.toFixed(2)}
            </p>
        </div>
    );
};

// Helper function to calculate metrics from the current order book summary and mid price history.
const calculateMetrics = (summary, midPrices) => {
    if (
        !summary ||
        !summary.bids ||
        !summary.asks ||
        summary.bids.length === 0 ||
        summary.asks.length === 0
    ) {
        return null;
    }
    const bestBid = summary.bids[0].price;
    const bestAsk = summary.asks[0].price;
    const spread = bestAsk - bestBid;
    const midPrice = (bestAsk + bestBid) / 2;
    const totalBidVolume = summary.bids.reduce((acc, level) => acc + level.quantity, 0);
    const totalAskVolume = summary.asks.reduce((acc, level) => acc + level.quantity, 0);
    let volatility = 0;
    if (midPrices.length > 1) {
        const mean = midPrices.reduce((acc, pt) => acc + pt.midPrice, 0) / midPrices.length;
        const variance =
            midPrices.reduce((acc, pt) => acc + Math.pow(pt.midPrice - mean, 2), 0) /
            (midPrices.length - 1);
        volatility = Math.sqrt(variance);
    }
    return { bestBid, bestAsk, spread, midPrice, totalBidVolume, totalAskVolume, volatility };
};

const OrderBookClient = () => {
    const [ws, setWs] = useState(null);
    const [connected, setConnected] = useState(false);
    const [summary, setSummary] = useState(null);
    const [messages, setMessages] = useState([]);
    const [orderId, setOrderId] = useState(1);
    const [orderType, setOrderType] = useState('');
    const [side, setSide] = useState('');
    const [price, setPrice] = useState('');
    const [quantity, setQuantity] = useState('');
    const [midPriceSeries, setMidPriceSeries] = useState([]);

    // Establish WebSocket connection on mount.
    useEffect(() => {
        const socket = new WebSocket('ws://localhost:8080');
        socket.onopen = () => {
            setConnected(true);
            console.log('Connected to server');
        };
        socket.onmessage = event => {
            try {
                const data = JSON.parse(event.data);
                // If message contains bids and asks, update summary and record mid price.
                if (data.bids && data.asks) {
                    setSummary(data);
                    if (data.bids.length > 0 && data.asks.length > 0) {
                        const midPrice = (data.bids[0].price + data.asks[0].price) / 2;
                        const now = new Date().toLocaleTimeString();
                        setMidPriceSeries(prev => [...prev, { time: now, midPrice }]);
                    }
                } else {
                    setMessages(prev => [...prev, data]);
                }
            } catch (err) {
                console.error('Error parsing message', err);
            }
        };
        socket.onerror = err => console.error('WebSocket error', err);
        socket.onclose = () => {
            setConnected(false);
            console.log('Disconnected from server');
        };
        setWs(socket);
        return () => {
            socket.close();
        };
    }, []);

    // Automatically request summary every second.
    useEffect(() => {
        if (ws && connected) {
            const interval = setInterval(() => {
                ws.send(JSON.stringify({ command: 'summary' }));
            }, 1000);
            return () => clearInterval(interval);
        }
    }, [ws, connected]);

    const sendOrder = () => {
        if (ws && connected) {
            const order = {
                id: orderId.toString(),
                type: orderType,
                side,
                price: Number(price),
                quantity: Number(quantity),
            };
            ws.send(JSON.stringify(order));
            setOrderId(prev => prev + 1);
        }
    };

    const metrics = calculateMetrics(summary, midPriceSeries);

    return (
        <div style={{ padding: '1rem', fontFamily: 'sans-serif' }}>
            <h1>Order Book Client</h1>

            {/* Display calculated metrics */}
            <MetricsPanel metrics={metrics} />

            {/* Order submission form */}
            <div style={{ marginBottom: '1rem' }}>
                <h3>Send Order</h3>
                <input
                    type="text"
                    placeholder="Order Type (GTC, IOC, FOK)"
                    value={orderType}
                    onChange={e => setOrderType(e.target.value)}
                    style={{ marginRight: '0.5rem' }}
                />
                <input
                    type="text"
                    placeholder="Side (buy, sell)"
                    value={side}
                    onChange={e => setSide(e.target.value)}
                    style={{ marginRight: '0.5rem' }}
                />
                <input
                    type="number"
                    placeholder="Price"
                    value={price}
                    onChange={e => setPrice(e.target.value)}
                    style={{ marginRight: '0.5rem', width: '80px' }}
                />
                <input
                    type="number"
                    placeholder="Quantity"
                    value={quantity}
                    onChange={e => setQuantity(e.target.value)}
                    style={{ marginRight: '0.5rem', width: '80px' }}
                />
                <button onClick={sendOrder} disabled={!connected}>
                    Send Order
                </button>
            </div>

            {/* Graphical order book summary */}
            <div style={{ marginBottom: '1rem' }}>
                <h3>Order Book Summary (Live Update)</h3>
                {summary ? (
                    <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                        <div style={{ flex: 1, marginRight: '1rem' }}>
                            <h4>Bids</h4>
                            <OrderBookDepth levels={summary.bids} side="bid" />
                        </div>
                        <div style={{ flex: 1, marginLeft: '1rem' }}>
                            <h4>Asks</h4>
                            <OrderBookDepth levels={summary.asks} side="ask" />
                        </div>
                    </div>
                ) : (
                    <p>No summary available.</p>
                )}
            </div>

            {/* Time-series chart for mid price */}
            <div style={{ marginTop: '1rem' }}>
                <h3>Order Book Time-Series (Mid Price)</h3>
                {midPriceSeries.length > 0 ? (
                    <OrderBookTimeSeriesChart midPriceSeries={midPriceSeries} />
                ) : (
                    <p>No time-series data available.</p>
                )}
            </div>

            {/* Message log */}
            <div style={{ marginTop: '1rem' }}>
                <h3>Messages</h3>
                <ul style={{ listStyle: 'none', padding: 0 }}>
                    {messages.map((msg, idx) => (
                        <li
                            key={idx}
                            style={{
                                marginBottom: '0.5rem',
                                border: '1px solid #ccc',
                                padding: '0.5rem',
                            }}
                        >
                            <pre>{JSON.stringify(msg, null, 2)}</pre>
                        </li>
                    ))}
                </ul>
            </div>

            <div>
                <strong>Status: </strong>
                {connected ? 'Connected' : 'Disconnected'}
            </div>
        </div>
    );
};

export default OrderBookClient;
