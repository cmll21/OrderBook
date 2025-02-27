import React, { useState, useEffect } from 'react';

const OrderBookClient = () => {
    const [ws, setWs] = useState(null);
    const [connected, setConnected] = useState(false);
    const [messages, setMessages] = useState([]);
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
                setMessages(prev => [...prev, data]);
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

    const sendSummary = () => {
        if (ws && connected) {
            const summaryCmd = { command: "summary" };
            ws.send(JSON.stringify(summaryCmd));
        }
    };

    return (
        <div style={{ padding: '1rem' }}>
            <h1>Order Book Client</h1>
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
            <div style={{ marginBottom: '1rem' }}>
                <button onClick={sendSummary} disabled={!connected}>
                    Request Order Book Summary
                </button>
            </div>
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
