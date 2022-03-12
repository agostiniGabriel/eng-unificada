const express = require('express');
const bodyParser = require('body-parser');
const config = require('config');
const consign = require('consign');

module.exports = () => {
    const automacaoApp = express();

    //Settings
    automacaoApp.set('port', process.env.PORT || config.get('server.port'));
    
    //Middlewares
    automacaoApp.use(bodyParser.json());

    //Endpoints
    consign({cwd: 'api'})
        .then('data')
        .then('controllers')
        .then('routes')
        .into(automacaoApp)

    return automacaoApp;
}
