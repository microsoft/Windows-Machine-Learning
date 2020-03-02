import * as Ajv from 'ajv';
import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ComboBox, IComboBoxOption } from 'office-ui-fabric-react/lib/ComboBox';
import { Icon } from 'office-ui-fabric-react/lib/Icon';
import * as React from 'react';
import { connect } from 'react-redux';

import IState from '../datastore/state';

import './KeyValueEditor.css';

const ajv = new Ajv({ allErrors: true, verbose: true });

interface IComponentProperties {
    actionCreator?: (keyValueObject: { [key: string]: string }) => {},
    getState: (state: IState) => { [key: string]: string },
    schema: any,

    // Redux properties
    keyValueObject?: { [key: string]: string },
    updateKeyValueObject?: (keyValueObject: { [key: string]: string }) => void,
}

interface IComponentState {
    caseInsensitiveSchema: any,
    keyErrors: { [key: string]: string },
}

class KeyValueEditor extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            caseInsensitiveSchema: this.toLowerCaseSchema(props.schema),
            keyErrors: {},
        };
    }

    public render() {
        const properties = this.props.schema.properties || {};
        const knownKeys = Object.keys(properties).filter(
            // Don't suggesting adding keys that already exist
            (x) => Object.keys(this.props.keyValueObject || {}).every((key) => !this.areSameKeys(key, x)));
        const options = knownKeys.map((key: string) => ({ key, text: key }));
        let rows: Array<React.ReactElement<HTMLDivElement>> = [];

        if (this.props.keyValueObject) {
            const [keyErrors, valueErrors] = this.validateSchema(this.state.caseInsensitiveSchema, this.props.keyValueObject);
            const schemaEntries = Object.entries(properties);

            rows = Object.keys(this.props.keyValueObject!).map((x: string) => {
                const lowerCaseKey = x.toLowerCase();
                const keyChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const key = value || option!.text;
                    if (Object.keys(this.props.keyValueObject!).includes(key)) {
                        this.setState((prevState: IComponentState, props: IComponentProperties) => (
                            {
                                keyErrors: {
                                    [lowerCaseKey]: prevState.keyErrors[lowerCaseKey] ? `${prevState.keyErrors[lowerCaseKey]} ` : 'duplicate key'
                                },
                            }
                        ));
                    } else {
                        this.props.updateKeyValueObject!(this.copyRenameKey(this.props.keyValueObject, x, key));
                    }
                };
                const valueChangedCallback = (option?: IComboBoxOption, index?: number, value?: string) => {
                    const newValue = value || option!.text;
                    this.props.updateKeyValueObject!({ ...this.props.keyValueObject!, [x]: newValue });
                };
                const removeCallback = () => {
                    const newObject = { ...this.props.keyValueObject! };
                    delete newObject[x];
                    this.props.updateKeyValueObject!(newObject);
                }

                let knownValues = [];
                const property = schemaEntries.find((prop) => this.areSameKeys(prop[0], x)) as [string, any];
                if (property && property[1].enum) {
                   knownValues = property[1].enum.map((key: string) => ({ key, text: key }));
                }

                const keyErrorsState = this.state.keyErrors[lowerCaseKey];
                return (
                    <div key={`${x}${keyErrorsState}`} className='DisplayFlex'>
                        { this.props.actionCreator &&
                            <Icon className='RemoveIcon' iconName='Cancel' onClick={removeCallback} />
                        }
                        <ComboBox
                            className='KeyValueBox'
                            allowFreeform={true}
                            text={x}
                            errorMessage={keyErrorsState || keyErrors[lowerCaseKey]}
                            options={options}
                            disabled={!this.props.actionCreator}
                            onChanged={keyChangedCallback}
                        />
                        <span className='KeyValueSeparator'>=</span>
                        <ComboBox
                            className='KeyValueBox'
                            allowFreeform={true}
                            text={this.props.keyValueObject![x]}
                            errorMessage={valueErrors[lowerCaseKey]}
                            options={knownValues}
                            disabled={!this.props.actionCreator}
                            onChanged={valueChangedCallback}
                        />
                    </div>
                );
            });
        }

        const addButtonDisabled = !(this.props.keyValueObject && Object.keys(this.props.keyValueObject).every((x) => !!x));
        const getSplitButtonClassNames = () => ({ splitButtonContainer: '.KeyValueEditorButtonContainer' });

        return (
            <div>
                {rows}
                { this.props.actionCreator &&
                    <DefaultButton
                        className='KeyValueEditorButtonContainer'
                        iconProps={{ iconName: 'Add' }}
                        disabled={addButtonDisabled}
                        onClick={this.addProp}
                        split={!!knownKeys.length}
                        menuProps={knownKeys.length ? {
                            items: options,
                            onItemClick: this.addProp,
                        } : undefined}
                        // The Office UI includes a spam element in it when split = true, which is not of display type block
                        // and ignores the parent width. We do a hack to get it to 100% of the parent width.
                        getSplitButtonClassNames={getSplitButtonClassNames}
                    />
                }
            </div>
        );
    }

    private validateSchema = (schema: any, data: { [key: string]: string }) => {
        const keyErrors = {};
        const valueErrors = {};
        if (!ajv.validate(schema, this.toLowerCaseObject(data))) {
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
        return Object.entries(obj).reduce((previousValue: any, keyValue: [string, string], currentIndex: number, acc: any[]) => {
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
        const properties = lowerCaseSchema.properties || {};
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
        return Object.entries(obj).reduce((acc: {}, keyValue: [string, any]) => {
            const [key, value] = keyValue;
            acc[key.toLowerCase()] = value.toString().toLowerCase();
            return acc;
        }, {});
    }

    private addProp = (event: any, item?: any) => {
        const key = item && item.key || '';
        if (this.props.keyValueObject) {
            this.props.updateKeyValueObject!({ ...this.props.keyValueObject, [key]: ''});
        }
    };
}

const mapStateToProps = (state: IState, ownProps: IComponentProperties) => ({
    keyValueObject: ownProps.getState(state),
});

const mapDispatchToProps = (dispatch: any, ownProps: IComponentProperties) => ({
    updateKeyValueObject: (keyValueObject: { [key: string]: string }) => dispatch(ownProps.actionCreator!(keyValueObject)),
})

export default connect(mapStateToProps, mapDispatchToProps)(KeyValueEditor as any);
