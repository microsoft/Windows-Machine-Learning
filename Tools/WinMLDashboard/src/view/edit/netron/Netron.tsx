import * as React from 'react';
import { connect } from 'react-redux';

import 'netron/src/view-render.css';
import 'netron/src/view-sidebar.css';
import 'netron/src/view.css';
import 'npm-font-open-sans/open-sans.css';

import { setDebugNodes, setInputs, setIntermediateOutputs, setMetadataProps, setModelInputs, setModelOutputs, setNodes, setOutputs, setProperties, setSelectedNode, setShowLeft, setShowRight } from '../../../datastore/actionCreators';
import { ModelProtoSingleton } from '../../../datastore/proto/modelProto';
import IState from '../../../datastore/state';
import './fixed-position-override.css';

import { DefaultButton } from 'office-ui-fabric-react/lib/Button';

import { onOpen } from '../../../native/menu'

const browserGlobal = window as any;

interface IComponentProperties {
    // Redux properties
    file: File,
    nodes: { [key: string]: any },
    setDebugNodes: typeof setDebugNodes,
    setInputs: typeof setInputs,
    setIntermediateOutputs: typeof setIntermediateOutputs,
    setMetadataProps: typeof setMetadataProps,
    setModelInputs: typeof setModelInputs,
    setModelOutputs: typeof setModelOutputs,
    setNodes: typeof setNodes,
    setOutputs: typeof setOutputs,
    setProperties: typeof setProperties,
    setSelectedNode: typeof setSelectedNode,
    setShowLeft: typeof setShowLeft,
    setShowRight: typeof setShowRight,
    showLeft: boolean,
    showRight: boolean
}

interface IComponentState {
    graph: any,
    isEdit: boolean,
    isOnnxModel: boolean,
    metadataProps: { [key: string]: string },
    properties: { [key: string]: string },
}

class Netron extends React.Component<IComponentProperties, IComponentState> {
    private proxiesRevoke: Array<() => void> = [];

    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            graph: {},
            isEdit: true,
            isOnnxModel: false,
            metadataProps: {},
            properties: {},
        }
        this.props.setShowRight(true);
    }

    public componentDidMount() {
        // Netron must be run after rendering the HTML
        if (!browserGlobal.host) {
            const s = document.createElement('script');
            s.src = "netron_bundle.js";
            s.async = true;
            s.onload = this.onNetronInitialized;
            document.body.appendChild(s);
        } else {
            this.onNetronInitialized();
        }
    }

    public componentWillUnmount() {
        for (const revoke of this.proxiesRevoke) {
            revoke();
        }
    }

    public UNSAFE_componentWillReceiveProps(nextProps: IComponentProperties) {
        if (!browserGlobal.host) {
            return;
        }
        if (nextProps.file && nextProps.file !== this.props.file) {
            browserGlobal.host._openFile(nextProps.file);
        }
    }

    // public shouldComponentUpdate(nextProps: IComponentProperties, nextState: IComponentState) {
    //     return false;  // Netron is a static page and all updates are handled by its JavaScript code
    // }

    public render() {
        const inlineStyle = {
            color:'lightgray',
            fontSize:'50px',
            fontStyle: 'oblique',
            top: '600px', 
            width: '400px'
        }
        return (
            // Instead of hardcoding the page, a div with dangerouslySetInnerHTML could be used to include Netron's content
            <div className='netron-root'>
                <div id='welcome' className='background' style={{display: 'block'}}>
                    <div className='center logo'>
                        <img className='logo absolute' src='winml_icon.ico' />
                        <img id='spinner' className='spinner logo absolute' src='spinner.svg' style={{display: 'none'}} />
                    </div>
                    <button id='open-file-button' className='center' disabled={true}> Open Model...</button>
                    <button id='openFileButton' className='center' style={{top: '200px', width: '100px'}} onClick={onOpen}>Open Model</button>
                    <p className='center' style={inlineStyle}>WinML Dashboard</p>
                    <input type="file" id="openFileButton" style={{display: 'none'}} multiple={false} accept=".onnx, .pb, .meta, .tflite, .keras, .h5, .json, .mlmodel, .caffemodel" />
                    <div style={{fontWeight: 'normal', color: '#e6e6e6', userSelect: 'none'}}>.</div>
                    <div style={{fontWeight: 600, color: '#e6e6e6', userSelect: 'none'}}>.</div>
                    <div style={{fontWeight: 'bold', color: '#e6e6e6', userSelect: 'none'}}>.</div>
                </div>
                <svg id='graph' className='graph' preserveAspectRatio='xMidYMid meet' width='100%' height='100%' />
                <div id='toolbar' className='toolbar' style={{position: 'absolute', top: '10px', left: '10px', display: 'none',}}>
                    <button id='model-properties-button' className='xxx' title='Model Properties'/>
                    <DefaultButton id='EditButton' text={'Edit/View'} onClick={this.toggleEditAndView}/>
                    <button id='zoom-in-button' className='icon' title='Zoom In'>
                        <svg viewBox="0 0 100 100" width="24" height="24">
                            <circle cx="50" cy="50" r="35" strokeWidth="8" stroke="#fff" />
                            <line x1="50" y1="38" x2="50" y2="62" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="12" strokeLinecap="square" stroke="#fff" />
                            <circle cx="50" cy="50" r="35" strokeWidth="4" />
                            <line x1="50" y1="38" x2="50" y2="62" strokeWidth="4" strokeLinecap="round" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="4" strokeLinecap="round" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="8" strokeLinecap="square" />
                        </svg>
                    </button>
                    <button id='zoom-out-button' className='icon' title='Zoom Out'>
                        <svg viewBox="0 0 100 100" width="24" height="24">
                            <circle cx="50" cy="50" r="35" strokeWidth="8" stroke="#fff" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="8" strokeLinecap="round" stroke="#fff" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="12" strokeLinecap="square" stroke="#fff" />
                            <circle cx="50" cy="50" r="35" strokeWidth="4" />
                            <line x1="38" y1="50" x2="62" y2="50" strokeWidth="4" strokeLinecap="round" />
                            <line x1="78" y1="78" x2="82" y2="82" strokeWidth="8" strokeLinecap="square" />
                        </svg>
                    </button>
                </div>
                <div id='sidebar' className='sidebar'>
                    <h1 id='sidebar-title' className='sidebar-title' />
                    <a href='javascript:void(0)' id='sidebar-closebutton' className='sidebar-closebutton'>&times;</a>
                    <div id='sidebar-content' className='sidebar-content' />
                </div>
            </div>
        );
    }


    private toggleEditAndView = () => {
        if(!this.state.isOnnxModel) {
            const convertDialogOptions = {
                message: 'Only ONNX model is editable. Convert it first.'
            }
            require('electron').remote.dialog.showMessageBox(convertDialogOptions)
            return;
        }
        if(this.state.isEdit) {
            this.setState({isEdit: false}, () => {document.getElementById('model-properties-button')!.click()})
            this.props.setShowLeft(false)
            this.props.setShowRight(false)
        }
        else {
            browserGlobal.view._sidebar.close();
            this.setState({isEdit: true})
            this.props.setShowLeft(true)
            this.props.setShowRight(true)
        }
    }
    private propsToObject(props: any) {
        return props.reduce((acc: { [key: string]: string }, x: any) => {
            // metadataProps uses key, while model.properties uses name
            acc[x.key || x.name] = x.value;
            return acc;
        }, {});
    }

    private valueListToObject(values: any) {
        return values.reduce((acc: { [key: string]: any }, x: any) => {
            const {name, ...props} = x;
            acc[name] = props;
            return acc;
        }, {});
    }

    private onNetronInitialized = () => {
        // Reset document overflow property
        document.documentElement!.style.overflow = 'initial';
        this.installProxies();
    }

    private installProxies = () => {
        // Install proxy on browserGlobal.view.loadBuffer and update the data store
        const loadBufferHandler = {
            apply: (target: any, thisArg: any, args: any) => {
                const [buffer, identifier, callback] = args;
                // Patch the callback to update our data store first
                return target.call(thisArg, buffer, identifier, (err: Error, model: any) => {
                    if (!err) {
                        this.updateDataStore(model);
                    }
                    return callback(err, model);
                });
            },
        };
        const loadBufferProxy = Proxy.revocable(browserGlobal.view.loadBuffer, loadBufferHandler);
        browserGlobal.view.loadBuffer = loadBufferProxy.proxy;
        this.proxiesRevoke.push(loadBufferProxy.revoke);

        const panelOpenHandler = {
            apply: (target: any, thisArg: any, args: any) => {
                if (this.props.nodes) {
                    const title = args[1];
                    if (title === 'Model Properties') {
                        this.props.setSelectedNode(title);
                        if(this.state.isEdit) {
                            return;
                        }
                    } else {
                        this.props.setSelectedNode(undefined);
                    }
                }
                return target.apply(thisArg, args);
            },
        };
        const panelOpenProxy = Proxy.revocable(browserGlobal.view._sidebar.open, panelOpenHandler);
        browserGlobal.view._sidebar.open = panelOpenProxy.proxy;
        this.proxiesRevoke.push(panelOpenProxy.revoke);

        const panelCloseHandler = {
            apply: (target: any, thisArg: any, args: any) => {
                this.props.setSelectedNode(undefined);
                return target.apply(thisArg, args);
            },
        };
        const panelCloseProxy = Proxy.revocable(browserGlobal.view._sidebar.close, panelCloseHandler);
        browserGlobal.view._sidebar.close = panelCloseProxy.proxy;
        this.proxiesRevoke.push(panelCloseProxy.revoke);

        const showNodePropertiesHandler = {
            apply: (target: any, thisArg: any, args: any) => {
                if(this.state.isEdit) {
                    const node = args[0];
                    this.props.setSelectedNode(node.graph.nodes.indexOf(node));
                    return;
                }
                return target.apply(thisArg, args);
            }
        }
        const showNodePropertiesProxy = Proxy.revocable(browserGlobal.view.showNodeProperties, showNodePropertiesHandler);
        browserGlobal.view.showNodeProperties = showNodePropertiesProxy.proxy;
        this.proxiesRevoke.push(showNodePropertiesProxy.revoke);
    }

    private getModelProperties(modelProto: any) {
        const opsetImport = (modelProto.opsetImport as Array<{domain: string, version: number}>)
            .reduce((acc, x) => {
                const opset = `opsetImport.${x.domain || 'ai.onnx'}`;
                return {
                    ...acc,
                    [opset]: x.version,
                };
            }, {});
        const properties = {
            ...opsetImport,
            docString: modelProto.docString,
            domain: modelProto.domain,
            irVersion: modelProto.irVersion,
            modelVersion: modelProto.modelVersion,
            producerName: modelProto.producerName,
            producerVersion: modelProto.producerVersion,
        }
        Object.entries(properties).forEach(([key, value]) => value || delete properties[key]);  // filter empty properties
        return properties;
    }

    private updateDataStore = (model: any) => {
        const proto = model._model;
        ModelProtoSingleton.proto = null;
        // FIXME What to do when model has multiple graphs?
        const graph = model.graphs[0];
        if (graph.constructor.name === 'OnnxGraph') {
            const getNames = (list: any[]): string[] => list.map((x: any) => x.name);
            const inputs = getNames(graph.inputs);
            const outputs = getNames(graph.outputs);
            this.props.setModelInputs(inputs);
            this.props.setModelOutputs(outputs);
            this.props.setInputs(this.valueListToObject(proto.graph.input));
            this.props.setOutputs(this.valueListToObject(proto.graph.output));
            const nodes = proto.graph.node.filter((x: any) => x.opType !== 'Constant');
            this.props.setNodes(nodes);
            const intermediateOutputs: string[] = []
            for (const nodeKey of Object.keys(nodes)) {
                intermediateOutputs.push(...(nodes[nodeKey].output))
            }
            this.props.setIntermediateOutputs(intermediateOutputs);

            this.props.setMetadataProps(this.propsToObject(proto.metadataProps));
            this.props.setProperties(this.getModelProperties(proto));
            this.props.setShowLeft(true);
            this.props.setShowRight(true);
            this.setState({isOnnxModel: true});
            this.setState({isEdit: true});

            

        } else {
            this.props.setShowLeft(false);
            this.props.setShowRight(false);
            this.props.setModelInputs(undefined);
            this.props.setModelOutputs(undefined);
            this.props.setNodes(undefined);
            this.props.setMetadataProps({});
            this.props.setProperties({});
            this.setState({isOnnxModel: false});
            this.setState({isEdit: false});
        }
        this.props.setSelectedNode(undefined);
        this.props.setDebugNodes({});
        ModelProtoSingleton.proto = proto;
    };
}

const mapStateToProps = (state: IState) => ({
    file: state.file,
    nodes: state.nodes,
    showLeft: state.showLeft,
    showRight: state.showRight,
});

const mapDispatchToProps = {
    setDebugNodes,
    setInputs,
    setIntermediateOutputs,
    setMetadataProps,
    setModelInputs,
    setModelOutputs,
    setNodes,
    setOutputs,
    setProperties,
    setSelectedNode,
    setShowLeft,
    setShowRight
}

export default connect(mapStateToProps, mapDispatchToProps)(Netron);
