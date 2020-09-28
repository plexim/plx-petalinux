/*
   Copyright (c) 2016 by Plexim GmbH
   All rights reserved.
*/

angular.module('rtBoxTest', ['ui.bootstrap', 'angularSpinner'])

.controller('modelCtrl', ['$scope', '$http', '$httpParamSerializerJQLike', function ($scope, $http, $httpParamSerializerJQLike) {

   $scope.hostname = '';
   $scope.server = {
      lost: false,
      error: false,
      notImplemented: false,
   };
   $scope.testStatus = {
      running: true
   };

   (function() {
      $http({
         url: '/cgi-bin/hostname.cgi',
         headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
         }
      })
      .then( function(response) {
         $scope.hostname = response.data;
      });
      $http({
         url: '/cgi-bin/boxtest.cgi',
         headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
         },
         timeout: 4000
      })
      .then( function(response) {
      }, function(response) {
         $scope.server.notImplemented = (response.status >= 400);
      })
   })();
}])

.controller('testCtrl', ['$scope', '$http', '$timeout', '$httpParamSerializerJQLike', '$uibModal', function ($scope, $http, $timeout, $httpParamSerializerJQLike, $uibModal) {

   $scope.testLog = 'Starting test application ...\n';

   $scope.refreshStatus = function() {
      $http({
         url: '/cgi-bin/teststatus.cgi',
         method: 'POST',
         headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
         },
         timeout: 2000
      })
      .then( function(response) {
         $scope.testLog += response.data.testLog;
         $scope.testStatus.running = (response.data.endOfTest == 0);
         if (!response.data.endOfTest)
         {
            $scope.refresh2(response);
         }
      }, function(response) {
         $scope.refresh2(response);
      });
   };

   $scope.refresh2 = function(response) {
      $scope.server.lost = (response.status < 100);
      $scope.server.error = (response.status >= 400 && response.status != 503);
      $scope.refreshTabPromise = $timeout(function() {
         $scope.refreshStatus();
      }, 500);
   };

   (function() {                                                                
      $scope.refreshTabPromise = $timeout(function() {
         $scope.refreshStatus();
      }, 2500);
   })();                                                                        

}])

.controller('ModalInstanceCtrl', ['$scope', '$uibModalInstance', function ($scope, $uibModalInstance) {

  $scope.close = function () {
    $uibModalInstance.close();
  };
}])

