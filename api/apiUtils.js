function randomizeResponse(mockData){
    const { responses } = mockData;
    let max = responses.length;
    return responses[Math.floor(Math.random() * (max))];
}

function validateBody(body,bodyTemplate){
    return Object.keys(body).every( property => bodyTemplate.hasOwnProperty(property));
}

const utilsPackage = {
    randomizeResponse: randomizeResponse,
    validateBody: validateBody
}

module.exports = utilsPackage;