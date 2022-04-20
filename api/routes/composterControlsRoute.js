module.exports = app => {
    const routeController = app.controllers.composterControlsController;

    app.route('/api/automation/avaliableActions')
    .get(routeController.getAvaliableActions);

    app.route('/api/automation/executeActions')
    .post(routeController.executeActions);

    app.route('/api/automation/systemStatus')
    .get(routeController.getSystemStatus);

    app.route('/api/automation/sensorData')
    .get(routeController.getSensorData);
}