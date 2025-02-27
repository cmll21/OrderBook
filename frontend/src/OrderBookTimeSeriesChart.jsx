// OrderBookTimeSeriesChart.jsx
import React, { useEffect, useState } from 'react';
import { Line } from 'react-chartjs-2';
import 'chart.js/auto';

const OrderBookTimeSeriesChart = ({ summary }) => {
    const [timeSeries, setTimeSeries] = useState([]);

    useEffect(() => {
        // When summary updates and has data, record a snapshot.
        if (summary && summary.bids.length > 0 && summary.asks.length > 0) {
            // Use current time for the snapshot label.
            const now = new Date().toLocaleTimeString();
            // Assuming bids are sorted descending and asks ascending.
            const bestBid = summary.bids[0].price;
            const bestAsk = summary.asks[0].price;
            setTimeSeries(prev => [...prev, { time: now, bestBid, bestAsk }]);
        }
    }, [summary]);

    const data = {
        labels: timeSeries.map(point => point.time),
        datasets: [
            {
                label: 'Best Bid Price',
                data: timeSeries.map(point => point.bestBid),
                borderColor: 'green',
                backgroundColor: 'rgba(0, 200, 0, 0.2)',
                fill: false,
            },
            {
                label: 'Best Ask Price',
                data: timeSeries.map(point => point.bestAsk),
                borderColor: 'red',
                backgroundColor: 'rgba(200, 0, 0, 0.2)',
                fill: false,
            }
        ]
    };

    const options = {
        responsive: true,
        scales: {
            x: {
                title: { display: true, text: 'Time' }
            },
            y: {
                title: { display: true, text: 'Price' }
            }
        }
    };

    return (
        <div style={{ width: '100%', height: '300px' }}>
            <Line data={data} options={options} />
        </div>
    );
};

export default OrderBookTimeSeriesChart;
