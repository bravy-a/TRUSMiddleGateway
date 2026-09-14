## TRUSMiddleGateway

TRUSMiddleGateway is a C++/Qt application that parses real-time market data from TRUS and synchronizes changed data to a database. The application keeps market state for each stock in-memory and periodically uploads data to the DB based on changes. A simple QML interface is included to monitor message activity and heartbeats.

Requirements: C++23, Qt 6.11, Qt Network, Qt SQL, Qt Quick/QML