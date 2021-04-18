const homeControllerCtor = function ($scope, $rootScope) {
    $rootScope.pageTitle = "Home";
}

const homeController = app.controller(
    'homeController',
    [
        '$scope', '$rootScope',
        homeControllerCtor
    ]);