/*
   Copyright (c) 2017 by Plexim GmbH
   All rights reserved.
*/

angular.module('rtBox', ['ui.bootstrap', 'gridshore.c3js.chart', 'xml-rpc'])

.factory('httpRequestInterceptor', ['$q', '$rootScope', function ($q, $rootScope) {
   return {
      'request': function(config) {
         if (!config.timeout) {
            config.timeout = 1000;
         }
         return config;
      },
      'response': function(response) {
         $rootScope.rtBoxServerStatus = response.status;
         if (response.status < 100 || response.status >= 400) {
            return $q.reject(response);
         } else {
            return response;
         }
      },
      'responseError': function(rejection) {
         $rootScope.rtBoxServerStatus = rejection.status;
         return $q.reject(rejection);
      }
   };
}])

.config(['$httpProvider', function($httpProvider) {
   $httpProvider.interceptors.push('httpRequestInterceptor');
}])

.controller('modelCtrl', ['$scope', '$http', '$httpParamSerializerJQLike', 'xmlrpc', function ($scope, $http, $httpParamSerializerJQLike, xmlrpc) {

   $scope.hostname = '';
   $scope.ip = '';
   $scope.serverStatusMsg = function() {
      var status = $scope.rtBoxServerStatus;
      if (status < 100)
         return 'Connection lost';
      else if (status >= 500)
         return 'Server error ('+status+')';
      else if (status >= 400)
         return 'Client error ('+status+')';
      else
         return '';
   }

   xmlrpc.config({
      hostName: 'http://' + window.location.hostname + ':9998'
   }); 
                                                          
   xmlrpc.callMethod('rtbox.hostname',[])
   .then(function(response) {
      $scope.hostname = response;
      return $http({                                                                      
         url: '/cgi-bin/ipstate.cgi',                                       
         headers: {                                                         
            'Content-Type': 'application/x-www-form-urlencoded'             
         }                                                                  
      });                                
   })
   .then(function(response) {
      $scope.ip = response.data.ip;                                      
   });                         
}])


/* https://blog.guya.net/2016/08/08/simple-server-polling-in-angularjs-done-right/ */

.controller('tabsCtrl', ['$scope', '$rootScope', '$http', '$interval', '$httpParamSerializerJQLike', '$uibModal',
            'xmlrpc', 
            function ($scope, $rootScope, $http, $interval, $httpParamSerializerJQLike, $uibModal, xmlrpc) {

   $scope.modalInstance;
   $scope.histLength = 21;
   
   $scope.activeTab = 0;
   $scope.ethernetModal = false;

   $scope.execTime = [ { 'Current': 0, 'Maximum': 0 } ];
   $scope.tempFpga = [ { 'Temperature': 0 } ];
   $scope.fanSpeed = [ { 'Speed': 0 } ];
   $scope.appfile = '';
   $scope.sampleTime = 0;
   $scope.execTimeHist = [];
   for (var i = 0; i < $scope.histLength; i++) {
       $scope.execTimeHist.push({ 'Current': 0, 'Maximum': 0 });
   }
   $scope.logPosition = 0;
   $scope.applicationLog = '';
   $scope.serialCpu = '';
   $scope.serialBoard = '';
   $scope.boardRevision = '';
   $scope.firmwareVersion = '';
   $scope.firmwareBuild = '';
   $scope.fpgaVersion = '';
   $scope.applicationVersion = '';
   $scope.ethernetMac = '';
   $scope.modelName = '';
   $scope.modelTimeStamp = 0;
   $scope.meta = {
      startAfterUpload: true,
      startOnFirstTrigger: false
   };
   $scope.startDisabled = false;
   $scope.stopDisabled = false;
   $scope.ipAddressTitle = 'IP Address';
   $scope.ipAddresses = '';

   $scope.rebootAlert = '';
   
   $scope.leds = {
   	error: false,
   	running: false,
   	ready: false,
   	power: false
   };
   
   $scope.voltageRanges = {
      analogIn: 0,   /* 1: +-10V, 2: +-5V */
      analogOut: 0,  /* 1: +-10V, 2: +-5V, 3: +-2.5V, 4: 0..10V, 5: 0..5V */
      digitalIn: 0, /* 13: 3.3V or 5V */
      digitalOut: 0 /* 11: 5V, 12: 3.3V */
   }
   
   $scope.netState = {
      ip: '',
      speed: 0,
      duplex: '',
      rx_errors: 0,
      tx_errors: 0,
      rxp: 0,
      txp: 0,
      collisions: 0
   }

   var loadTime = 1000, // reload after 1 s
      loadPromise;
      
   var getData = function() {
      switch ($scope.activeTab) {
         case 1:
            refreshFrontPanel();
            break;
         case 2:
            refreshRearPanel();
            break;
         case 3:
            refreshStatus();
            break;
         case 4:
            refreshInfoTab();
            break;
         default:
            break;
      }
      refreshExecutionTime();
   }
      
   var cancelNextLoad = function() {
      // $timeout.cancel(loadPromise);
      $interval.cancel(loadPromise);
   };

   var nextLoad = function() {
      cancelNextLoad();
      // loadPromise = $timeout(getData, loadTime);
      loadPromise = $interval(getData, loadTime);
   }
      
   $scope.$on('$destroy', function() {
      cancelNextLoad();
   });

   $scope.$watch('activeTab', function(newTab) {
      switch (newTab) {
         case 0:
            break;
         case 1:
            refreshFrontPanel();
            break;
         case 2:
            refreshRearPanel();
            break;
         case 3:
            refreshStatus();
            break;
         case 4:
            refreshInfoTab();
            break;
         default:
            break;
      }
   });
	
   $scope.openEthernetModal = function() {
      $scope.ethernetModal = true;
      var modalInstance = $uibModal.open({
         animation: true,
         templateUrl: 'partials/ethernet.html',
	      size: 'md',
         scope: $scope
      });
      refreshRearPanel();
      modalInstance.closed.then( function() {
         $scope.ethernetModal = false;
      });
   };

   $scope.printVoltageRange = function(index) {
      switch (index) {
         case 1: return '-10 V ... 10 V'; break;
         case 2: return '-5 V ... 5 V'; break;
         case 3: return '-2.5 V ... 2.5 V'; break;
         case 4: return '0 ... 10 V'; break;
         case 5: return '0 ... 5 V'; break;
         case 6: return '-2.5V ... 7.5 V'; break;
         case 11: return '5 V'; break;
         case 12: return '3.3 V'; break;
         case 13: return '3.3 V or 5 V'; break;
         default: return '';
      }
   }
   
   $scope.highlightVoltageRange = function(index) {
      switch (index) {
         case 1:
         case 2:
         case 3:
         case 4:
         case 11: return true;
         default: return false;
      }
   }

   var refreshFrontPanel = function() {
         $scope.leds.error = false;
      $http({
         url: '/cgi-bin/front-panel.cgi',
         method: 'POST',
         headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
         }/*,
         timeout: connectionTimeout*/
      })
      .then( function(response) {
         if (response.data.leds) {
            $scope.leds = response.data.leds;
         }
         if (response.data.voltageRanges) {
            $scope.voltageRanges = response.data.voltageRanges;
         }
      });
   }

   var refreshRearPanel = function() {
      if ($scope.ethernetModal) {
         refreshNetstate();
      }
   }

   var refreshNetstate = function() {                                         
      $http({
         url: '/cgi-bin/netstate.cgi',
         headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
         }
      })
      .then( function(response) {
         $scope.speed = response.data.speed;
         $scope.duplex = response.data.duplex;
         $scope.rx_errors = response.data.rx_errors;
         $scope.tx_errors = response.data.tx_errors;
         $scope.rxp = response.data.rxp;
         $scope.txp = response.data.txp;
         $scope.collisions = response.data.collisions;
      });
   };

   var refreshStatus = function() {
      xmlrpc.callMethod('rtbox.status',[
         $scope.logPosition, $scope.modelTimeStamp
      ])
      .then( function(response) {
         $scope.tempFpga[0].Temperature = response.temperature;
         $scope.fanSpeed[0].Speed = response.fanSpeed;
         if (response.clearLog == 1) {
            $scope.modelTimeStamp = response.modelTimeStamp;
            $scope.applicationLog = "";
            $scope.logPosition = response.logPosition;
            $scope.applicationLog += response.applicationLog;
         }
         else if (response.logPosition < $scope.logPosition) {
            response.logPosition = 0;
            $scope.applicationLog = "";
         }
         else {
            $scope.logPosition = response.logPosition;
            $scope.applicationLog += response.applicationLog;
         }
      });
   }

   var refreshModelInfo = function() {                                  
      xmlrpc.callMethod('rtbox.querySimulation',[])
      .then( function(response) {                 
         $scope.modelName = response.modelName;
         if (response.sampleTime) {                                
            $scope.sampleTime = parseFloat(response.sampleTime.toPrecision(6)) * 1e9;
         } else {
            $scope.sampleTime = 0;
         }
         $scope.modelTimeStamp = response.modelTimeStamp;
         $scope.applicationVersion = response.applicationVersion;
      });
   }                                                                            

   var refreshInfoTab = function() {
      refreshModelInfo();
      if (!$scope.serialCpu) {
         xmlrpc.callMethod('rtbox.serials', [])
         .then( function(response) {                  
            $scope.serialCpu = response.cpu;                                     
            $scope.serialBoard = response.board;                                     
            $scope.ethernetMac = response.mac;
            $scope.boardRevision = response.boardRevision;
            $scope.firmwareVersion = response.firmwareVersion;
            $scope.firmwareBuild = response.firmwareBuild;
            $scope.fpgaVersion = response.fpgaVersion;
            $scope.ipAddresses = response.ipAddresses.join(', ');
            if (response.ipAddresses.length > 1) {
               $scope.ipAddressTitle = 'IP Addresses';
            } else {
               $scope.ipAddressTitle = 'IP Address';
            }
         });
      }
   }

   var refreshExecutionTime = function() {
      xmlrpc.callMethod('rtbox.queryCounter',[])
      .then( function(response) {
         if ($scope.sampleTime) {
            $scope.execTime[0].Current = response.runningCycleTime/$scope.sampleTime*100;
            $scope.execTime[0].Maximum = response.maxCycleTime/$scope.sampleTime*100;
            $scope.execTimeHist.push( {
               'Current': $scope.execTime[0].Current,
               'Maximum': $scope.execTime[0].Maximum
            } );
         }
         if ($scope.modelTimeStamp != response.modelTimeStamp) {
            refreshModelInfo();
         }
         if ($scope.execTimeHist.length > $scope.histLength) {
            $scope.execTimeHist.splice(0, $scope.execTimeHist.length-$scope.histLength);
         }
      });
   }
   
   $scope.resetExecutionTime = function() {
      xmlrpc.callMethod('rtbox.resetCounter',[])
   }

   $scope.loadAndRestart = function(file) {
      $scope.stopExecution( function() {
         if ($scope.meta.startAfterUpload) {
            $scope.uploadFiles(file, $scope.startExecution);
         } else {
            $scope.uploadFiles(file);
         }
      });
   }
      
   $scope.stopExecution = function(callbackSuccess) {
      $scope.stopDisabled = true;
      cancelNextLoad();
      $http({
         url: '/cgi-bin/stop.cgi',
         headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
         },
         timeout: 3000
      })
      .then( function() {
         nextLoad();
         $scope.stopDisabled = false;
         if (callbackSuccess) {                                
            callbackSuccess();                                 
         }                                                     
      }, function(rejection) {
         nextLoad();
         $scope.stopDisabled = false;
      });
   }

   $scope.startExecution = function() {
      $scope.startDisabled = true;
      cancelNextLoad();
      $http({
         url: '/cgi-bin/start.cgi',
         method: 'POST',
         data: $httpParamSerializerJQLike({
            'startOnFirstTrigger' : $scope.meta.startOnFirstTrigger.toString()
         }),
         headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
         },
         timeout: 5000
      })
      .then( function() {
         nextLoad();
         $scope.startDisabled = false;
      }, function(rejection) {
         nextLoad();
         $scope.startDisabled = false;
         if (rejection.status == 500) {
            $rootScope.rtBoxServerStatus = 200;
            alert(rejection.data);
         }
      });
   }



   $scope.uploadFiles = function(file, callbackSuccess) {
      $http.post('/cgi-bin/upload.cgi', file, {
         withCredentials: true,
         headers: { 'Content-Type': undefined },
         timeout: 10000,
         transformRequest: angular.identity
      })
      .then( function() {
         if (callbackSuccess) {                                
            callbackSuccess();                                 
         }                                                     
      }, function (rejection) {
         alert(rejection.data);
      });
   }

   $scope.formatExecTime = function(value, ratio) {
      if (value == $scope.execTime[0].Current) {
         return value.toFixed(0);
      } else {
         return;
      }
   }

   $scope.rebootSystem = function(callbackSuccess) {
      if (confirm('Are you sure you want to reboot the system?')){
         $scope.rebootDisabled = true;
         $scope.rebootAlert = 'Rebooting ...';
         cancelNextLoad();
         xmlrpc.callMethod('rtbox.reboot',['reboot']);
         setTimeout(function() {
            location.reload(true);
         }, 25000);
      }
   }

   $scope.structTempFpga = [
   	{ 'id': 'Temperature', 'type': 'gauge' }
   ];

   $scope.structFanSpeed = [
   	{ 'id': 'Speed', 'type': 'gauge' }
   ];

   $scope.structExecTime = [
   	{ 'id': 'Maximum', 'type': 'gauge', 'color': '#ff7f0e' },
   	{ 'id': 'Current', 'type': 'gauge', 'color': '#1f77b4' }
   ];
   
   $scope.structExecTimeHist = [
   	{ 'id': 'Maximum', 'type': 'line', 'color': '#ff7f0e' },
   	{ 'id': 'Current', 'type': 'line', 'color': '#1f77b4' }
   ];
   
   $scope.formatTempFpga = function(value, ratio) {
      return value.toFixed(1);
   }

   $scope.formatFanSpeed = function(value, ratio) {
      return value.toFixed(0);
   }

   $scope.openModal = function(htmlTemplate) {
      var modalInstance = $uibModal.open({
         animation: true,
         templateUrl: 'partials/'+htmlTemplate,
	 size: 'lg',
      });
  }

  // getData(); // start the polling process
  loadPromise = $interval(getData, loadTime);

}])

.directive("ngFile", [function () {
   return {
      scope: {
         ngFile: "=",
         ngFilename: "="
      },
      link: function (scope, element, attrs) {
         element.bind("change", function (changeEvent) {
            scope.$apply(function () {
               var target = changeEvent.target;
               scope.ngFilename = angular.element(target).val().replace(/\\/g, '/').replace(/.*\//, '');
               scope.ngFile = new FormData();
               scope.ngFile.append(attrs.name, target.files[0]);
            });
         });
      }
   }
}])

.directive('ngConfirmReboot', [
    function(){
        return {
            link: function (scope, element, attr) {
               scope.rebootAlert = '';
                var msg = attr.ngConfirmReboot || "Are you sure you want to reboot the system?";
                var clickAction = attr.confirmedClick;
                element.on('click',function (event) {
                    if ( window.confirm(msg) ) {
                        scope.$eval(clickAction)
                    }
                });
            }
        };
}])

