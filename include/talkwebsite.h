

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW TALKER DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">

    <!-- Bootstrap CSS -->
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.6.0/dist/css/bootstrap.min.css" integrity="sha384-B0vP5xmATw1+K9KRQjQERJvTumQW0nPEzvF6L/Z6nronJ3oUOFUFpCjEUQouq2+l" crossorigin="anonymous">
  <style>
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
    .scrolltable {
      display:block;
      height : 400px;
      overflow-y : scroll;
    }
  </style>    
</head>
<body>
  <nav class="navbar navbar-dark bg-primary">
    <span class="navbar-brand  mr-auto mb-0 h1">ESP-NOW TALKER DASHBOARD</span>


<div class="nav-item btn-group">

  <button type="button" class="btn btn-info dropdown-toggle" data-toggle="dropdown" 
                        aria-haspopup="true" aria-expanded="false">
    Status
  </button>
  <div class="dropdown-menu dropdown-menu-right dropdown-menu-dark ">
    <div class="dropdown-item d-flex justify-content-between ">Version <small class="" id="sts_version" href="#">Menu item</small></div>
    <div class="dropdown-divider"></div>
    <div class="dropdown-item d-flex justify-content-between ">Local IP <small class="" id="sts_localIP" href="#">Menu item</small></div>
    <div class="dropdown-item d-flex justify-content-between ">Master IP <small class="" id="sts_masterIP" href="#">Menu item</small></div>
    <div class="dropdown-item d-flex justify-content-between ">Externel IP <small class="" id="sts_externIP" href="#">Menu item</small></div>
    <div class="dropdown-item d-flex justify-content-between ">Gateway IP <small class="" id="sts_gatewayIP" href="#">Menu item</small></div>
    <div class="dropdown-item d-flex justify-content-between ">Mac address <small class="" id="sts_mac" href="#">Menu item</small></div>
    <div class="dropdown-item d-flex justify-content-between ">Channel <small class="" id="sts_channel" href="#">Menu item</small></div>
  </div>
</div>
</nav>    
<main role="main" class="container">

  <div class="album py-5">
    <div class="container">

      <div class="row">
        <div class="col-md-8">
          <div class="card mb-4 humidity shadow-sm">
            <div class="card-header">
              <i class="fas fa-tint"></i> Node Overview
            </div>
            <div class="card-body ">
           
              <div class="scrolltable">
              <table class="table table-sm table-hover table-striped">
    <thead class="thead-light">
      <tr>
        <th>Status</th>
        <th>Node</th>
        <th>Name</th>
        <th>Externel IP</th>
        
      </tr>
    </thead>
    <tbody  class="nodes">
      <tr>
        <td><i class="far fa-check-circle"></i></td>
        <td>123456789012</td>
        <td>1543.225.25.2</td>
        <td>John</td>
      </tr>
      <tr>
        <td><i class="far fa-check-circle"></i></td>
        <td>123456789012</td>
        <td>1543.225.25.2</td>
        <td>John</td>
      </tr>
      <tr>
        <td><i class="far fa-check-circle"></i></td>
        <td>123456789012</td>
        <td>1543.225.25.2</td>
        <td>John</td>
      </tr>
    </tbody>
  </table>
  </div>
            </div>
          </div>
        </div>


        <div class="col-md-4">
          <div class="card mb-4 temperature shadow-sm">
                      <div class="card-header">
              <i class="fas fa-comments"></i> Messages
            </div>
            <div class="card-body">
              <div class="scrolltable" >
               <table class="table table-sm table-borderless">
                 <tbody class="messages">
                 </tbody>
              </table>             
              </div> 
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</main><!-- /.container -->



<script src="https://code.jquery.com/jquery-3.5.1.slim.min.js" integrity="sha384-DfXdz2htPH0lsSSs5nCTpuj/zy4C+OGpamoFVy38MVBnE+IbbVYUew+OrCXaRkfj" crossorigin="anonymous"></script>
<script src="https://cdn.jsdelivr.net/npm/bootstrap@4.6.0/dist/js/bootstrap.bundle.min.js" integrity="sha384-Piv4xVNRyMGpqkS2by6br4gNJ7DXjqk09RmUpJ8jgGtD7zP9yug3goQfGII0yAns" crossorigin="anonymous"></script>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  $(".messages").append("<tr><td>"+e.data+"</td></tr>");
  console.log("message", e.data);
 }, false);


 source.addEventListener('new_status', function(e) {
  console.log("new_status", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("sts_version").innerHTML = obj.version.toFixed(2);
  document.getElementById("sts_localIP").innerHTML = obj.localIP;
  document.getElementById("sts_masterIP").innerHTML = obj.masterIP;
  document.getElementById("sts_externIP").innerHTML = obj.externelIP;
  document.getElementById("sts_gatewayIP").innerHTML = obj.gatewayIP;
  document.getElementById("sts_mac").innerHTML = obj.mac;
  document.getElementById("sts_channel").innerHTML = obj.channel;
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("nodes").innerHTML = "";
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt"+obj.id).innerHTML = obj.readingId;
  document.getElementById("rh"+obj.id).innerHTML = obj.readingId;
 }, false);
}
</script>
</body>
</html>)rawliteral";
