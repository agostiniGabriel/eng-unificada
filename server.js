const automacaoApp = require('./config/express')();
const port = automacaoApp.get('port');

automacaoApp.listen(port, ()=>{
    console.log('Tudo certo por aqui 😁');
})