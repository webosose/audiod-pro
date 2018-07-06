'use strict';

Object.defineProperty(exports, "__esModule", {
	value: true
});

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

/* eslint-disable no-console */
/* global console */

var refs = {};

var adjustPath = function adjustPath(path) {
	if (path.slice(-1) !== '/') {
		path += '/';
	}
	return path;
};

var LS2Request = function () {
	function LS2Request() {
		_classCallCheck(this, LS2Request);

		this.bridge = null;
		this.cancelled = false;
		this.subscribe = false;
	}

	_createClass(LS2Request, [{
		key: 'send',
		value: function send(_ref) {
			var _ref$service = _ref.service,
			    service = _ref$service === undefined ? '' : _ref$service,
			    _ref$method = _ref.method,
			    method = _ref$method === undefined ? '' : _ref$method,
			    _ref$parameters = _ref.parameters,
			    parameters = _ref$parameters === undefined ? {} : _ref$parameters,
			    _ref$onSuccess = _ref.onSuccess,
			    onSuccess = _ref$onSuccess === undefined ? null : _ref$onSuccess,
			    _ref$onFailure = _ref.onFailure,
			    onFailure = _ref$onFailure === undefined ? null : _ref$onFailure,
			    _ref$onComplete = _ref.onComplete,
			    onComplete = _ref$onComplete === undefined ? null : _ref$onComplete,
			    _ref$subscribe = _ref.subscribe,
			    subscribe = _ref$subscribe === undefined ? false : _ref$subscribe;

			if ((typeof window === 'undefined' ? 'undefined' : _typeof(window)) !== 'object' || !window.PalmServiceBridge) {
				/* eslint no-unused-expressions: ["error", { "allowShortCircuit": true }]*/
				onFailure && onFailure({ errorCode: -1, errorText: 'PalmServiceBridge not found.', returnValue: false });
				onComplete && onComplete({ errorCode: -1, errorText: 'PalmServiceBridge not found.', returnValue: false });
				console.error('PalmServiceBridge not found.');
				return;
			}

			if (this.ts && refs[this.ts]) {
				delete refs[this.ts];
			}

			this.subscribe = subscribe;
			if (this.subscribe) {
				parameters.subscribe = this.subscribe;
			}
			if (parameters.subscribe) {
				this.subscribe = parameters.subscribe;
			}

			// eslint-disable-next-line no-undef
			this.ts = performance.now();
			refs[this.ts] = this;

			// eslint-disable-next-line no-undef
			this.bridge = new PalmServiceBridge();
			this.bridge.onservicecallback = this.callback.bind(this, onSuccess, onFailure, onComplete);
			this.bridge.call(adjustPath(service) + method, JSON.stringify(parameters));

			return this;
		}
	}, {
		key: 'callback',
		value: function callback(onSuccess, onFailure, onComplete, msg) {
			if (this.cancelled) {
				return;
			}
			var parsedMsg = void 0;
			try {
				parsedMsg = JSON.parse(msg);
			} catch (e) {
				parsedMsg = {
					errorCode: -1,
					errorText: msg,
					returnValue: false
				};
			}

			if (parsedMsg.errorCode || parsedMsg.returnValue === false) {
				if (onFailure) {
					onFailure(parsedMsg);
				}
			} else if (onSuccess) {
				onSuccess(parsedMsg);
			}

			if (onComplete) {
				onComplete(parsedMsg);
			}
			if (!this.subscribe) {
				this.cancel();
			}
		}
	}, {
		key: 'cancel',
		value: function cancel() {
			this.cancelled = true;
			if (this.bridge) {
				this.bridge.cancel();
				this.bridge = null;
			}

			if (this.ts && refs[this.ts]) {
				delete refs[this.ts];
			}
		}
	}]);

	return LS2Request;
}();

exports.default = LS2Request;