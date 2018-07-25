import * as Ajv from 'ajv';
import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ComboBox, IComboBoxOption } from 'office-ui-fabric-react/lib/ComboBox';
import * as React from 'react';
import { connect } from 'react-redux';

import IState from '../datastore/state';

import './KeyValueEditor.css';

const ajv = new Ajv({ allErrors: true, verbose: true });

interface IComponentProperties {
    actionCreator: (keyValueObject: { [key: string]: string }) => {},
    getState: (state: IState) => { [key: string]: string },
    schema: any,

    // Redux properties
    keyValueObject?: { [key: string]: string },
    updateKeyValueObject?: (keyValueObject: { [key: string]: string }) => void,
}

interface IComponentState {
    caseInsensitiveSchema: any,
}

class KeyValueEditorComponent extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            caseInsensitiveSchema: this.toLowerCaseSchema(props.schema),
        };
    }

    public render() {
        let rows = [];
        if (this.props.keyValueObject) {
            const [keyErrors, valueErrors] = this.validateSchema(this.state.caseInsensitiveSchema, this.props.keyValueObject);
            const schemaEntries = Object.entries(this.props.schema.properties);
            const knownKeys = Object.keys(this.props.schema.properties).filter(
                // Don't suggesting adding keys that already exist
                (x) => Object.keys(this.props.keyValueObject!).every((key) => !this.areSameKeys(key, x)));
            const options = knownKeys.map((key: string) => ({ key, text: key }));

            rows = Object.keys(this.props.keyValueObject!).reduce((acc: any[], x: string) => {
                const keyChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const key = value || option!.text;
                    this.props.updateKeyValueObject!(this.copyRenameKey(this.props.keyValueObject, x, key));
                };
                const valueChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const newValue = value || option!.text;
                    this.props.updateKeyValueObject!({ ...this.props.keyValueObject!, [x]: newValue });
                };

                let knownValues = [];
                const property = schemaEntries.find((prop) => this.areSameKeys(prop[0], x)) as [string, any];
                if (property && property[1].enum) {
                   knownValues = property[1].enum.map((key: string) => ({ key, text: key }));
                }

                acc.push(
                    <div key={x} id='KeyValueItem'>
                        <ComboBox
                            className='KeyValueBox'
                            allowFreeform={true}
                            text={x}
                            errorMessage={keyErrors[x.toLowerCase()]}
                            options={options}
                            onChanged={keyChangedCallback}
                        />
                        <span className='KeyValueSeparator'>=</span>
                        <ComboBox
                            className='KeyValueBox'
                            allowFreeform={true}
                            text={this.props.keyValueObject![x]}
                            errorMessage={valueErrors[x.toLowerCase()]}
                            options={knownValues}
                            onChanged={valueChangedCallback}
                        />
                    </div>
                );
                return acc;
            }, []);
        }

        const addDisabled = !(this.props.keyValueObject && Object.keys(this.props.keyValueObject).every((x) => !!x))
        const removeDisabled = !(this.props.keyValueObject && Object.keys(this.props.keyValueObject).length)

        return (
            <div>
                {rows}
                <DefaultButton className='KeyValueEditorButton' iconProps={{ iconName: 'Add' }} disabled={addDisabled} onClick={this.addMetadataProp} />
                <DefaultButton className='KeyValueEditorButton' iconProps={{ iconName: 'Cancel' }} disabled={removeDisabled} onClick={this.removeMetadataProp} />
            </div>
        );
    }

    private validateSchema = (schema: any, data: { [key: string]: string }) => {
        const keyErrors = {};
        const valueErrors = {};
        const lowerCaseData = {...data};
        this.toLowerCaseObject(lowerCaseData);
        if (!ajv.validate(schema, lowerCaseData)) {
            ajv.errors!.forEach((error: Ajv.ErrorObject) => {
                if (error.schemaPath.startsWith('#/properties/')) {
                    valueErrors[error.schemaPath.split('/', 3)[2]] = error.message!;
                } else if (error.keyword === 'additionalProperties') {
                    const params = error.params as Ajv.AdditionalPropertiesParams;
                    keyErrors[params.additionalProperty] = 'unknown property';
                }
            });
        }
        return [keyErrors, valueErrors];
    }

    private areSameKeys = (a: string, b: string) => {
        return a.toLowerCase() === b.toLowerCase();
    }

    private copyRenameKey = (obj: any, oldKey: string, newKey: string) => {
        return Object.entries(obj).reduce((acc: any[], keyValue: [string, string]) => {
            const [key, value] = keyValue;
            acc[key === oldKey ? newKey : key] = value;
            return acc;
        }, {});
    }

    private renameKey = (obj: any, oldKey: string, newKey: string) => {
        if (oldKey !== newKey) {
            obj[newKey] = obj[oldKey];
            delete obj[oldKey];
        }
    }

    private toLowerCaseSchema = (schema: any) => {
        // Hacky way of performing case insensitive validation. JSON schema has no case insensitive enum,
        // so we convert the schema and object being validated to lowercase before validating.
        // This code assumes the object schema has a "properties" property.
        const lowerCaseSchema = JSON.parse(JSON.stringify(schema));
        const properties = lowerCaseSchema.properties;
        Object.keys(properties).forEach((key: string) => {
            const propertyEnum = properties[key].enum;
            if (propertyEnum) {
                propertyEnum.forEach((x: string, i: number, arr: any[]) => arr[i] = x.toLowerCase());
            }
            this.renameKey(properties, key, key.toLowerCase());
        });
        return lowerCaseSchema;
    }

    private toLowerCaseObject = (obj: { [key: string]: string }) => {
        Object.entries(obj).forEach((keyValue: [string, string]) => {
            const [key, value] = keyValue;
            obj[key] = value.toLowerCase();
            this.renameKey(obj, key, key.toLowerCase());
        });
    }

    private addMetadataProp = () => {
        if (this.props.keyValueObject) {
            this.props.updateKeyValueObject!({ ...this.props.keyValueObject, '': ''});
        }
    };

    private removeMetadataProp = () => {
        const newObject = { ...this.props.keyValueObject! };
        const keys = Object.keys(newObject);
        delete newObject[keys[keys.length - 1]];
        this.props.updateKeyValueObject!(newObject);
    };
}

const mapStateToProps = (state: IState, ownProps: IComponentProperties) => ({
    keyValueObject: ownProps.getState(state),
});

const mapDispatchToProps = (dispatch: any, ownProps: IComponentProperties) => ({
    updateKeyValueObject: (keyValueObject: { [key: string]: string }) => dispatch(ownProps.actionCreator(keyValueObject)),
})

const KeyValueEditor = connect(mapStateToProps, mapDispatchToProps)(KeyValueEditorComponent as any);
export default KeyValueEditor;
