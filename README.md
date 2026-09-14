## TRUSMiddleGateway

TRUSMiddleGateway is a C++/Qt application that parses real-time market data from TRUS and synchronizes changed data to a database. The application keeps market state for each stock in-memory and periodically uploads data to the DB based on changes. A simple QML interface is included to monitor message activity and heartbeats.

<img width="496" height="486" alt="Screenshot 2026-09-14 114332" src="https://github.com/user-attachments/assets/207b9e09-e8e2-4f39-9525-35ca40938ea1" />

Requirements: C++23, Qt 6.11, Qt Network, Qt SQL, Qt Quick/QML
