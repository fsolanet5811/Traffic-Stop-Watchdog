const app = angular.module('app', ['ngMaterial', 'ngRoute']);

const home = {
    templateUrl: 'views/homeView.html',
    controller: 'homeController'
};

const resources = {
    templateUrl: 'views/resourcesView.html',
    controller: 'resourcesController'
}

const diagrams = {
    templateUrl: 'views/diagramsView.html',
    controller: 'diagramsController'
}

app.config(function ($routeProvider, $locationProvider) {
    $locationProvider.hashPrefix('');
    $routeProvider.when('/', home)
    .when('/home', home)
    .when('/resources', resources)
    .when('/diagrams', diagrams)
    .otherwise({
        redirectTo: '/'
    });
})