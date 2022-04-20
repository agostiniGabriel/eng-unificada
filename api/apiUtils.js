function randomizeResponse(mockData){
    const { responses } = mockData;
    let max = responses.length;
    return responses[Math.floor(Math.random() * (max))];
}

function validateBody(body,bodyTemplate){
    return body && Object.keys(bodyTemplate).every( property => body.hasOwnProperty(property));
}

const utilsPackage = {
    randomizeResponse: randomizeResponse,
    validateBody: validateBody
}

module.exports = utilsPackage;