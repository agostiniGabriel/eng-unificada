const { json } = require('express/lib/response');

module.exports = app =>{
    const controller = {};
    const updateReadingsResponses = app.data.updateReadings;
    const utils = require('../apiUtils');

    const requiredBodyTemplate = {
        payload: {
            temp: 0 ,
            humidity:0 ,
            pH: 0
        }
    }

    controller.updateReadings = (req,res) => {
        const { body } = req;
        utils.validateBody(body,requiredBodyTemplate) ? res.status(200).json(updateReadingsResponses["201"]) : res.status(400).json(updateReadingsResponses["400"]);
    }

    return controller;
}