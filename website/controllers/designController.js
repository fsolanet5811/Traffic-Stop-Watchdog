const designControllerCtor = function ($scope, $rootScope) {
    $rootScope.pageTitle = "Diagrams";
}

const designController = app.controller(
    'designController',
    [
        '$scope', '$rootScope',
        designControllerCtor
    ]);