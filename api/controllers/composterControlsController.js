module.exports = app =>{
    const controller = {};
    const avaliableActions = app.data.avaliableActions;
    const responsesCodes = app.data.updateReadings;
    const mockESP8266 = app.data.esp8266Mock;
    const ESP8266_URL =  process.env.ESP8266_IP;
    const utils = require('../apiUtils');
    const axios = require('axios');

    controller.getAvaliableActions = (req,res) => {
        res.status(200).json(avaliableActions.actions);
    }

    controller.executeActions =  (req,res) => {
        const { body } = req;
        const { actionsResquestBodyTemplate } = avaliableActions;
        if(utils.validateBody(body.data , actionsResquestBodyTemplate.data)){
            const { actionsList } = body.data;
            if(!actionsList.every( x => avaliableActions.actions.find(action => action.actionId == x.actionId))) res.status(422).json(responsesCodes["422"]);

            if(ESP8266_URL){

            } else {
                const details = body.data.actionsList.map((x) =>{
                    return { actionRequested: x.actionId , result: { ...mockESP8266.executeAction} };
                });
                const mockedResponse = {
                    ...responsesCodes["200"],
                    details: [...details]
                }
                res.status(200).json(mockedResponse);
            }
        } 
        
        res.status(400).json(responsesCodes["400"]);
    }

    controller.getSystemStatus = (req,res) => {
        if(ESP8266_URL){

            
        }
        const mockedResponse = {
            sucess:true,
            statusCode:200,
            ...mockESP8266.systemStatus
        }
        res.status(200).json(mockedResponse)
    } 

    controller.getSensorData = (req,res) => {
        if(ESP8266_URL){

            
        }
        const mockedResponse = {
            sucess:true,
            statusCode:200,
            ...mockESP8266.sensorData
        }
        res.status(200).json(mockedResponse)

    }


    return controller;
}