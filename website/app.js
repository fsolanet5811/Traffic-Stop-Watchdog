const app = angular.module('app', ['ngMaterial', 'ngRoute']);

const home = {
    templateUrl: 'views/homeView.html',
    controller: 'homeController'
};

const about = {
    templateUrl: 'views/aboutView.html',
    controller: 'aboutController'
}

const design = {
    templateUrl: 'views/designView.html',
    controller: 'designController'
}

app.config(function ($routeProvider, $locationProvider) {
    $locationProvider.hashPrefix('');
    $routeProvider.when('/', home)
    .when('/home', home)
    .when('/about', about)
    .when('/design', design)
    .otherwise({
        redirectTo: '/'
    });
})