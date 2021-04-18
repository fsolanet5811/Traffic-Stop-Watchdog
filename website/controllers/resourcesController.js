const resourcesControllerCtor = function ($scope, $rootScope) {
    $rootScope.pageTitle = "Resources";
}

const resourcesController = app.controller(
    'resourcesController',
    [
        '$scope', '$rootScope',
        resourcesControllerCtor
    ]);