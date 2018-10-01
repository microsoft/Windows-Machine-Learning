export default {
    // https://github.com/onnx/onnx/blob/master/docs/MetadataProps.md
    type: 'object',

    additionalProperties: false,
    properties: {
        'Image.BitmapPixelFormat': {
            enum: [
                'Gray8',
                'Rgb8',
                'Bgr8',
                'Rgba8',
                'Bgra8',
            ],
        },
        'Image.ColorSpaceGamma': {
            enum: [
                'Linear',
                'SRGB',
            ],
        },
        'Image.NominalPixelRange': {
            enum: [
                'NominalRange_0_255',
                'Normalized_0_1',
                'Normalized_1_1',
                'NominalRange_16_235',
            ],
        },

        author: {
            type: 'string',
        },
        company: {
            type: 'string',
        },
        converted_from: {
            type: 'string',
        },
        licence: {
            type: 'string',
        },
        licence_url: {
            format: 'uri',
            type: 'string',
        },
        model_author: {
            type: 'string',
        },
        model_license: {
            type: 'string',
        },
    },
};
