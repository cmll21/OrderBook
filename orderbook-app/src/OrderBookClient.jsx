// OrderBookClient.jsx
import React, { useState, useEffect } from 'react';
import { Line } from 'react-chartjs-2';
import 'chart.js/auto';

// Component to display a time-series chart of best bid and ask prices.
const OrderBookTimeSeriesChart = ({ summary }) => {
    const [timeSeries, setTimeSeries] = useState([]);

    useEffect(() => {
        if (
            summary &&
            summary.bids &&
            summary.asks &&
            summary.bids.length > 0 &&
            summary.asks.length > 0
        ) {
            const now = new Date().toLocaleTimeString();
            // Assuming bids are sorted descending and asks ascending.
            const bestBid = summary.bids[0].price;
            const bestAsk = summary.asks[0].price;
            setTimeSeries(prev => [...prev, { time: now, bestBid, bestAsk }]);
        }
    }, [summary]);

    const data = {
        labels: timeSeries.map(pt => pt.time),
        datasets: [
            {
                label: 'Best Bid Price',
                data: timeSeries.map(pt => pt.bestBid),
                borderColor: 'green',
                backgroundColor: 'rgba(0, 200, 0, 0.2)',
                fill: false,
            },
            {
                label: 'Best Ask Price',
                data: timeSeries.map(pt => pt.bestAsk),
                borderColor: 'red',
                backgroundColor: 'rgba(200, 0, 0, 0.2)',
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

// Component to render a depth chart for one side of the order book.
const OrderBookDepth = ({ levels, side }) => {
    // Determine maximum volume for relative bar width.
    const maxVolume =
        levels && levels.length > 0 ? Math.max(...levels.map(level => level.quantity)) : 1;
    // Use green for bids and red for asks.
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
                // If the message contains bids and asks, treat it as a summary update.
                if (data.bids && data.asks) {
                    setSummary(data);
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

    // Automatically request order book summary every 100 ms.
    useEffect(() => {
        if (ws && connected) {
            const interval = setInterval(() => {
                ws.send(JSON.stringify({ command: 'summary' }));
            }, 100);
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

    return (
        <div style={{ padding: '1rem', fontFamily: 'sans-serif' }}>
            <h1>Order Book Client</h1>

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

            {/* Time-series chart */}
            <div style={{ marginTop: '1rem' }}>
                <h3>Order Book Time-Series (Best Bid & Ask)</h3>
                {summary ? (
                    <OrderBookTimeSeriesChart summary={summary} />
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
