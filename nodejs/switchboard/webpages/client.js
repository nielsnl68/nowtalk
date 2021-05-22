$(document).ready(function () {
    'use strict'

    //const newbadgeAdd = document.getElementById('newbadgeAdd');

    const socket = io();
    var _errormsg = $("<div class='alert alert-danger m-1 p-1  text-center small'  role='alert'>Server has been disconnected.</div>");
    $("#main").prepend(_errormsg);
    $("#newBadgeAlert").hide();
    $("#editBadgeAlert").hide();

    // client-side
    socket.on("connect", () => {
        if (_errormsg != null) {
            _errormsg.hide();
            $("#main").remove(_errormsg);
            _errormsg = null;
        }
        socket.emit('newBadge', false);
    });

    socket.on("disconnect", (reason) => {
        if (reason === "io server disconnect") {
            // the disconnection was initiated by the server, you need to reconnect manually
            socket.connect();
        }
        if (_errormsg != null) {
            _errormsg = $("<div class='alert alert-danger m-1 p-1  text-center small'  role='alert'>Server has been disconnected.</div>");
            $("#main").prepend(_errormsg);
        }
    });

    // Get room and users
    socket.on('config', (obj) => {
        $("#sts_switchboardName").text(obj.switchboardName);
        $("#sts_version").text(obj.version + "/" + obj.bridge.version);
        $("#sts_externIP").text(obj.externelIP || "<None>");
        $("#sts_mac").text(obj.bridge.mac);
        $("#sts_channel").text(obj.bridge.channel);
        $("#sts_bridgeid").text(obj.bridge.bridgeID);
    });

    socket.on('newBadge', (obj) => {
        $("#listBadges tr.table-info").removeClass('table-info');
        $("#editBadgeAlert").hide();

        if (obj === false) {
            $("#newBadgeAlert").hide();;
            $("#newBadgeConfirm").hide();
            $("#newbadgeYes").off('click');
            $("#newbadgeClose").off('click');
            $("#newBadgeForm").off('submit');
            return;
        }

        $("#newBadgeConfirm").show();
        $("#newBadgeForm").hide();
        $("#newBadgeAlert").show();
        $("#newbadgeClose").on("click", function () {
            $("#newbadgeClose").off("click");
            $("newBadgeForm").off('submit');
            $("#newbadgeYes").off('click');
            $("#badgeid").val("");
            $("#username").val("");

            $("#newBadgeAlert").hide();
            socket.emit('newBadge', false);
        });
        $("#newbadgeYes").on('click', function () {
            socket.emit('newBadge', true);
            $("#newBadgeConfirm").hide();
            $("#newBadgeForm").show();
            $("#badgeid").val("");
            $("#username").val("");
            $("#newBadgeForm").on("submit", function (event) {
                let form = document.getElementById("newBadgeForm");
                event.preventDefault()
                event.stopPropagation()
                if (!form.checkValidity()) {
                    return;
                }
                form.classList.add('was-validated')
                socket.emit('newBadge',
                    {
                        badgeid: $("#badgeid").val(),
                        username: $("#username").val(),
                    });

                $("#newbadgeClose").trigger("click");
            });
        });
    });

    socket.on('msg', (kind, msg) => callAddMessage(kind, msg));

    socket.on('msgs', (list) => {
        $("#msgbody").empty();
        list.forEach((item) => {
            callAddMessage(item[0],item[1]);
        });
    });

    socket.on('update', (list, time) => {

        let table = $("#listBadges");
        list.forEach((item) => {
            //       return { mac: useHexMac ? this._mac : this.mac, status: this.status, ip: this.ip, name: this.name, time: this.timestamp };
            let status_icon = 'handshake'; // item.status.toString(16);
            let status_color = 'secondary';
            let status_hint = 'Status code: ' + item.status.toString(16);
            let isActive = ((item.status & 0x01) == 0x01);
            if ((item.status) == 0x00) {
                status_color = "primary";
                status_hint = "Handshake";
            } else if ((item.status & 0x70) == 0x10) {
                status_icon = "user-alt";
                if (isActive) status_color = 'success';
                status_hint = "Local User";
            } else if ((item.status & 0x78) == 0x18) {
                status_icon = "people-arrows";
                if (isActive) status_color = 'success';
                status_hint = "Outside user";
            } else if ((item.status & 0x70) == 0x20) {
                status_icon = "users";
                if (isActive) status_color = 'success';
                status_hint = "Family or friends";
            } else if ((item.status & 0x72) == 0x02 || (item.status & 0xFF) == 0x80) {
                status_icon = "eye";
                if (isActive) status_color = 'success';
                status_hint = "Guest";
            } else {
                status_icon = "exclamation-circle";
                status_color = "info";
            }
            if ((item.status & 0x80) == 0x80) {
                status_icon += "-slash";
                status_color = 'danger';
                status_hint += 'disabled';
            }
            let status = '<i class="text-' + status_color + ' fas fa-' + status_icon+'" title="'+status_hint+'"></i>';

            /*
            <i class="far fa-comments"></i>
            <i class="fas fa-bullhorn"></i>
            <i class="fas fa-deaf"></i>
            <i class="fas fa-globe"></i>
            <i class="fas fa-microphone-alt-slash"></i>
            <i class="fas fa-user"></i>
            <i class="fas fa-user-alt-slash"></i>
            <i class="fas fa-user-clock"></i>
            <i class="fas fa-users"></i>
            
            <i class="fas fa-people-arrows"></i>
            <i class="fas fa-mail-bulk"></i>
            <i class="fas fa-handshake-slash"></i>
            */

            let x = item.time;
            let progress = '<div class="progress-bar" role="progressbar" aria-valuenow="' + x + '" aria-valuemin="0" aria-valuemax="100" style="width: ' + x + '%"></div>';
            progress = "<div class='progress'>" + progress + "</div>";
            let newRow = "<td  class='py-0 text-center align-middle'></td><td>" + item.mac.substr(1) + "</td><td>" + item.name + "</td><td>" + item.ip + "</td><td class='align-middle'>" + progress + "</td>";
            if (table.has("#" + item.mac).length == 0) {
                table.append("<tr id='" + item.mac + "'></tr>");
            }
            let row = $("#" + item.mac);
            row.empty();
            row.append(newRow);
            row.attr('data-time', time);
            row.attr('data-status', item.status);
            let xy = row.children().first();
            xy.append(status);
        });
        table.children('[data-time!=' + time + ']').remove();
    });


    function callAddMessage(kind, msg) {
        let row = $("<div class='row'></div>");
        let _msg = $("<div class='m-0  p-0 small'  role='alert'>" + msg + "</div>");
        switch (kind) {
            case 'recv':
                _msg.addClass("col-10 px-1 mt-1 text-success border rounded-end alert-secondary  ");
                break;
            case 'send':
                row.append("<div class='col-2'></div>");
                _msg.addClass("text-end col-10 mt-1 px-1 text-primary rounded-start alert-secondary ");
                break;
            default:
                _msg.addClass("col-12 text-" + kind);
        }
        row.append(_msg);
        $("#msgBody").append(row);
        var elem = document.getElementById('msgBody');
        elem.scrollTop = elem.scrollHeight;
    }

    function checkBadgeID(code) {
        let baseChars = "0123456789AbCdEfGhIjKlMnOpQrStUvWxYz";
        const baseCount = baseChars.length + 1;
        baseChars += "aBcDeFgHiJkLmNoPqRsTuVwXyZ";
        let length = code.length;
        var count = 0;
        var arr = [];
        if (length != 8) return false;
        for (let index = 6; index >= 0; index--) {
            let chr = code.charAt(index);
            count += baseChars.indexOf(chr);
            arr.push([count, baseChars.indexOf(chr), chr]);
        }
        let crc = count % baseCount, chr = code.charAt(7);
        return crc === baseChars.indexOf(chr);
    }

    const email = document.getElementById("badgeid");

    badgeid.addEventListener("input", function (event) {
        let id = $(badgeid).val();
        if (!checkBadgeID(id)) {
            email.setCustomValidity("This is an invalid badgeID. The code is case sensitive!");
        } else {
            email.setCustomValidity("");
        }
    });

    $("#listBadges").on("click", "tr", function (event) {
        event.preventDefault();

        $("#listBadges tr.table-info").removeClass('table-info');
        $(this).addClass("table-info");
        $("#editBadgeAlert").show();
        $("#editBadgeAlert").attr('data-id', $(this).attr('id'));
        let status = $(this).data('status') * 1;
        if ((status & 0x80) !== 0) {
            $("#changeDisable").removeClass('btn-primary').addClass('btn-outline-primary');
        } else {
            $("#changeDisable").addClass('btn-primary').removeClass('btn-outline-primary');
        }
        if ((status & 0x20)!== 0) {
            $("#changeFriend").removeClass('btn-primary').addClass('btn-outline-primary');
        } else {
            $("#changeFriend").addClass('btn-primary').removeClass('btn-outline-primary');
        }
        if ((status & 0x10) !== 0) {
            $("#changeFriend").attr('disabled', 'disabled' )
        } else {
            $("#changeFriend").removeAttr('disabled', 'disabled')
        }

    });
    $(".toggleButton").on('click', (e, x, y, z) => {
        let button = $(this.activeElement);
        e.preventDefault();
        button.toggleClass('btn-primary');
        button.toggleClass('btn-outline-primary');
        socket.emit('editBadge', button.parent().data('id'), button.data('action'), button.hasClass('btn-primary'));

    });
    
    $("#editbadgeClose").on('click', (e) => {
        e.preventDefault();
        $("#listBadges tr.table-info").removeClass('table-info');
        $("#editBadgeAlert").hide();
        $("#editBadgeAlert").removeAttr('data-id');
    });

});