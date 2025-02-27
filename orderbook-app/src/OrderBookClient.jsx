import React, { useState, useEffect } from 'react';

const OrderBookClient = () => {
    const [ws, setWs] = useState(null);
    const [connected, setConnected] = useState(false);
    const [messages, setMessages] = useState([]);
    const [summary, setSummary] = useState(null); // State for order book summary
    const [orderId, setOrderId] = useState(1);
    const [orderType, setOrderType] = useState('');
    const [side, setSide] = useState('');
    const [price, setPrice] = useState('');
    const [quantity, setQuantity] = useState('');

    // Establish WebSocket connection on mount
    useEffect(() => {
        const socket = new WebSocket('ws://localhost:8080');
        socket.onopen = () => {
            setConnected(true);
            console.log("Connected to server");
        };
        socket.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                // If message contains bids and asks, treat as a summary update.
                if (data.bids && data.asks) {
                    setSummary(data);
                } else {
                    setMessages(prev => [...prev, data]);
                }
            } catch (err) {
                console.error("Error parsing message", err);
            }
        };
        socket.onerror = (err) => console.error("WebSocket error", err);
        socket.onclose = () => {
            setConnected(false);
            console.log("Disconnected from server");
        };

        setWs(socket);

        // Clean up on unmount
        return () => {
            socket.close();
        };
    }, []);

    // Automatically request a summary every 1 second
    useEffect(() => {
        if (ws && connected) {
            const interval = setInterval(() => {
                sendSummary();
            }, 1000); // Update every 1000ms (1 second)
            return () => clearInterval(interval);
        }
    }, [ws, connected]);

    // Send order over WebSocket
    const sendOrder = () => {
        if (ws && connected) {
            const order = {
                id: orderId.toString(),
                type: orderType,
                side,
                price: Number(price),
                quantity: Number(quantity)
            };
            ws.send(JSON.stringify(order));
            setOrderId(prev => prev + 1);
        }
    };

    // Request order book summary from the server
    const sendSummary = () => {
        if (ws && connected) {
            const summaryCmd = { command: "summary" };
            ws.send(JSON.stringify(summaryCmd));
        }
    };

    // Compute total volumes from summary data
    const computeTotals = () => {
        let totalBids = 0, totalAsks = 0;
        if (summary) {
            summary.bids.forEach(level => {
                totalBids += level.quantity;
            });
            summary.asks.forEach(level => {
                totalAsks += level.quantity;
            });
        }
        return { totalBids, totalAsks };
    };

    const { totalBids, totalAsks } = computeTotals();

    return (
        <div style={{ padding: '1rem' }}>
            <h1>Order Book Client</h1>

            {/* Order Form */}
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
                    style={{ marginRight: '0.5rem' }}
                />
                <input
                    type="number"
                    placeholder="Quantity"
                    value={quantity}
                    onChange={e => setQuantity(e.target.value)}
                    style={{ marginRight: '0.5rem' }}
                />
                <button onClick={sendOrder} disabled={!connected}>
                    Send Order
                </button>
            </div>

            {/* Summary GUI */}
            {summary && (
                <div style={{ marginBottom: '1rem' }}>
                    <h3>Order Book Summary</h3>
                    <div style={{ marginBottom: '1rem' }}>
                        <h4>Bids (Total Volume: {totalBids})</h4>
                        <table border="1" cellPadding="5">
                            <thead>
                            <tr>
                                <th>Price</th>
                                <th>Quantity</th>
                            </tr>
                            </thead>
                            <tbody>
                            {summary.bids.map((level, idx) => (
                                <tr key={idx}>
                                    <td>{level.price}</td>
                                    <td>{level.quantity}</td>
                                </tr>
                            ))}
                            </tbody>
                        </table>
                    </div>
                    <div>
                        <h4>Asks (Total Volume: {totalAsks})</h4>
                        <table border="1" cellPadding="5">
                            <thead>
                            <tr>
                                <th>Price</th>
                                <th>Quantity</th>
                            </tr>
                            </thead>
                            <tbody>
                            {summary.asks.map((level, idx) => (
                                <tr key={idx}>
                                    <td>{level.price}</td>
                                    <td>{level.quantity}</td>
                                </tr>
                            ))}
                            </tbody>
                        </table>
                    </div>
                </div>
            )}

            {/* General Messages */}
            <div>
                <h3>Messages</h3>
                <ul style={{ listStyle: 'none', padding: 0 }}>
                    {messages.map((msg, idx) => (
                        <li key={idx} style={{ marginBottom: '0.5rem', border: '1px solid #ccc', padding: '0.5rem' }}>
                            <pre>{JSON.stringify(msg, null, 2)}</pre>
                        </li>
                    ))}
                </ul>
            </div>

            <div>
                <strong>Status: </strong>
                {connected ? "Connected" : "Disconnected"}
            </div>
        </div>
    );
};

export default OrderBookClient;
