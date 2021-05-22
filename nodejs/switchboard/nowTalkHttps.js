
/*jshint esversion: 10 */
"use strict";

const http = require("http");
const fsx = require('fs');
const { parse } = require('querystring');
const process = require('process');
const socketio = require('socket.io');

const {
    userJoin,
    userLeave,
    userCount
} = require('./utils/users');

const MAX_MSGS = 100;

class NowTalkHttps {
    constructor(main) {
        // super();
        this.main = main;
        this.config = main.config;
        this.nodes = [];
        this.msgs = [];
        this.clients = [];
        this.timer = false;
        this.onUpdateBadges = this.onUpdateBadges.bind(this);
        this.requestListener = this.requestListener.bind(this);

        this.server = http.createServer(this.requestListener);
        this.io = socketio(this.server);

        this.io.on('connection', socket => {
            const user = userJoin(socket.id, 'username', 'room');
            // handle newBadge return
            socket.on("newBadge", data => {
                console.warn('socket_newBadge', data);
                if (typeof data === "boolean") {
                    socket.broadcast.emit("newBadge", false);
                }
                this.main.emit("web_newBadge", data);
            });

            // Runs when client disconnects
            socket.on('disconnect', () => {
                const user = userLeave(socket.id);
            });

            socket.emit('config', this.config);
            socket.emit('msgs', this.msgs);
            this.onUpdateBadges(socket);
        });

        if (main.config.webAddress === "*") {
            this.server.listen(main.config.webPort);
        } else {
            this.server.listen(main.config.webPort, main.config.webAddress);
        }
        this.timer = setInterval(this.onUpdateBadges, 2500, this.io);
    }

    stop() {
        clearInterval(this.timer);
        this.clients.forEach(connection => {
            if (!connection) return;
            connection.end();
        });
        this.server.close();
    }

    requestListener(req, res) {
        //     res.setHeader('Access-Control-Allow-Origin', this.config.sitebaseURL);
        res.setHeader('Access-Control-Request-Method', 'GET');
        //        res.setHeader('Access-Control-Allow-Methods', 'OPTIONS,POST,GET');
        res.setHeader('Access-Control-Allow-Headers', '*');
        /*         if (req.method === 'OPTIONS') return res.end();
                if (req.method === 'POST' && req.url === '/p') {
        
                    var body = "", http = this;
                    req.on("data", function (chunk) {
                        body += chunk;
                    });
        
                    req.on("end", function () {
                        http.postRequest(req, res, parse(body));
                        res.end('ok');
                    });
                }
         */
        if (req.method === 'GET' && req.url === '/client.js') {
            res.setHeader('Content-type', 'text/javascript');
            res.writeHead(200);
            return res.end(fsx.readFileSync("./webpages/client.js"));
        }
        switch (req.url) {
            case "":
            case "/":
                res.setHeader("Content-Type", "text/html");
                res.writeHead(200);
                //       this.indexFile =;
                res.end(fsx.readFileSync("./webpages/index.html"));
                break;
            default:
                res.writeHead(404);
                res.end(JSON.stringify({ error: "Resource not found" }));
        }
    }

    updateConfig(connections = false) {
        this.io.emit('config',  this.config);
    }

    addNewBadge(data) {
        this.io.emit('newBadge', data);
        return userCount() !== 0;
    }

    addMessage(kind, msg) {
        while (this.msgs.length > MAX_MSGS) {
            this.msgs.shift();
        }
        this.msgs.push([kind, msg]);
        this.io.emit('msg', kind, msg);
    }

    onUpdateBadges(socket) {
        let list = [];

        for (const [key, value] of Object.entries(this.main.users)) {
            list.push(value.info(true));
        }
        socket.emit('update', list, process.hrtime.bigint().toString());
    }

}

module.exports = NowTalkHttps;