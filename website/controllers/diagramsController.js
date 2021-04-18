const diagramsControllerCtor = function ($scope, $rootScope) {
    $rootScope.pageTitle = "Diagrams";
}

const diagramsController = app.controller(
    'diagramsController',
    [
        '$scope', '$rootScope',
        diagramsControllerCtor
    ]);