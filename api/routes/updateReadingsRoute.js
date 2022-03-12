module.exports = app => {
    const routeController = app.controllers.updateReadingsController;

    app.route('/api/automation/updateReadings')
    .post(routeController.updateReadings);
}