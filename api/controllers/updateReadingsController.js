module.exports = app =>{
    const controller = {};
    const updateReadingsResponses = app.data.updateReadings;
    const utils = require('../apiUtils');
    const axios = require('axios');

    const requiredBodyTemplate = {
        payload: {
            temp: 0 ,
            humidity:0 ,
            pH: 0
        }
    }

    controller.updateReadings = (req,res) => {
        const { body } = req;
        if(utils.validateBody(body,requiredBodyTemplate)){
            const now = new Date();

            const formattedBody = {
                data: {
                    updateDate: now.toLocaleString('pt-BR'),
                    sensorReading: body.payload
                }
            }
        
            axios
                .post(
                    process.env.BACKEND_DADOS, 
                    formattedBody
                )
                .then((response)=>{  
                    const { status , data } = response;
                    status === 200 ?  res.status(201).json(updateReadingsResponses["201"]) : res.status(status).json(data);
                })
                .catch((error) => {
                    const { status , data} = error.response;
                    res.status(status).json(data)
                });

        } else {
           res.status(400).json(updateReadingsResponses["400"]);
        }
    }

    return controller;
}