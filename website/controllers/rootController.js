const rootControllerCtor = function ($rootScope, $location) {
    $rootScope.go = function(route) {
        $location.path(route);
    }
}

const rootController = app.controller('rootController',
    [
        '$rootScope',
        '$location',
        rootControllerCtor
    ]);