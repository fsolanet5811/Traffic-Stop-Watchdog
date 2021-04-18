const aboutControllerCtor = function ($scope, $rootScope) {
    $rootScope.pageTitle = "About";
}

const aboutController = app.controller(
    'aboutController',
    [
        '$scope', '$rootScope',
        aboutControllerCtor
    ]);